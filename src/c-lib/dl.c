/**
 * GS1 barcode encoder library
 *
 * @author Copyright (c) 2021 GS1 AISBL.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "gs1encoders.h"
#include "enc-private.h"
#include "debug.h"
#include "ai.h"
#include "dl.h"


#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof(x[0]))


/*
 *  Set of characters that are permissible in URIs, including percent
 *
 */
static const char *uriCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=%";


/*
 * List of DL pkeys
 *
 * These are only used for find the beginning of the DL pathinfo.
 *
 */
static const char* dl_pkeys[] = {
	"00",		// SSCC
	"01",		// GTIN; qualifiers 22,10,21 or 235
	"253",		// GDTI
	"255",		// GCN
	"401",		// GINC
	"402",		// GSIN
	"414",		// LOC NO.; qualifiers=254 or 7040
	"417",		// PARTY; qualifiers=7040
	"8003",		// GRAI
	"8004",		// GIAI; qualifiers=7040
	"8006",		// ITIP; qualifiers=22,10,21
	"8010",		// CPID; qualifiers=8011
	"8013",		// GMN
	"8017",		// GSRN - PROVIDER; qualifiers=8019
	"8018",		// GSRN - RECIPIENT; qualifiers=8019
};

static bool isDLpkey(char* p) {
	size_t i;
	assert(p);
	for (i = 0; i < SIZEOF_ARRAY(dl_pkeys); i++)
		if (strcmp(p, dl_pkeys[i]) == 0)
			return true;
	return false;
}


static size_t URIunescape(char *out, size_t maxlen, const char *in, const size_t inlen) {

	size_t i, j;
	char hex[3] = { 0 };

	assert(in);
	assert(out);

	for (i = 0, j = 0; i < inlen && j < maxlen; i++, j++) {
		if (i < inlen - 2 && in[i] == '%' && isxdigit(in[i+1]) && isxdigit(in[i+2])) {
			hex[0] = in[i+1];
			hex[1] = in[i+2];
			out[j] = (char)strtoul(hex, NULL, 16);
			i += 2;
		}
		else {
			out[j] = in[i];
		}
	}
	out[j] = '\0';

	return j;

}


/*
 * Convert DL data to regular AI data string with # = FNC1
 *
 * This performs a lightweight parse, sufficient for extracting the AIs for
 * validation and HRI purposes.
 *
 * It does not validate the structure of the DL URI nor the data relationships
 * between the extracted AIs.
 *
 * Extraction using convenience strings for GS1 keys are not supported.
 *
 */
bool gs1_parseDLuri(gs1_encoder *ctx, char *dlData, char *dataStr) {

	char *p, *r, *e, *ai, *outai, *outval;
	char *pi = NULL;	// Path info
	char *qp = NULL;	// Query params
	char *fr = NULL;	// Fragment
	char *dp = NULL;	// DL path info
	bool ret;
	size_t i;
	size_t ailen, vallen;
	bool fnc1req = true;
	const struct aiEntry *entry;
	char aival[MAX_AI_LEN+1];	// Unescaped AI value

	assert(ctx);
	assert(dlData);

	*dataStr = '\0';
	*ctx->errMsg = '\0';
	ctx->errFlag = false;

	DEBUG_PRINT("\nParsing DL data: %s\n", dlData);

	p = dlData;

	if (strspn(p, uriCharacters) != strlen(p)) {
		strcpy(ctx->errMsg, "URI contains illegal characters");
		goto fail;
	}

	if (strlen(p) >= 8 && strncmp(p, "https://", 8) == 0)
		p += 8;
	else if (strlen(p) >= 7 && strncmp(p, "http://", 7) == 0)
		p += 7;
	else {
		strcpy(ctx->errMsg, "Scheme must be http:// or https://");
		goto fail;
	}

	DEBUG_PRINT("  Scheme %.*s\n", (int)(p-dlData-3), dlData);

	if (((r = strchr(p, '/')) == NULL) || r-p < 1) {
		strcpy(ctx->errMsg, "URI must contain a domain and path info");
		goto fail;
	}

	DEBUG_PRINT("  Domain: %.*s\n", (int)(r-p), p);

	pi = p = r;					// Skip the domain name

	// Query parameter marker delimits end of path info
	if ((qp = strchr(pi, '?')) != NULL)
		*qp++ = '\0';

	DEBUG_PRINT("  Path info: %s\n", pi);

	// Search backwards from the end of the path info looking for an
	// "/AI/value" pair where AI is a DL primary key
	while ((r = strrchr(pi, '/')) != NULL) {

		*p = '/';				// Restore original pair separator
							// Clobbers first character of path
							// info on first iteration

		// Find start of AI
		*r = '\0';				// Chop off value
		p = strrchr(pi, '/'); 			// Beginning of AI
		*r = '/';				// Restore original AI/value separator
		if (!p)					// At beginning of path
			break;

		DEBUG_PRINT("      %s\n", p);

		entry = gs1_lookupAIentry(ctx, p+1, (size_t)(r-p-1));
		if (!entry)
			break;

		if (isDLpkey(entry->ai)) {		// Found root of DL path info
			dp = p;
			break;
		}

		*p = '\0';

	}

	if (!dp) {
		strcpy(ctx->errMsg, "No GS1 DL keys found in path info");
		goto fail;
	}

	DEBUG_PRINT("  Stem: %.*s\n", (int)(dp-dlData), dlData);

	DEBUG_PRINT("  DL path info: %s\n", dp);

	// Process each AI value pair in the DL path info
	p = dp;
	while (*p) {
		assert(*p == '/');
		p++;
		r = strchr(p, '/');
		assert(r);

		// AI is known to be valid since we previously walked over it
		ai = p;
		ailen = (size_t)(r-p);
		assert((entry = gs1_lookupAIentry(ctx, ai, ailen)) != NULL);

		if ((p = strchr(++r, '/')) == NULL)
			p = r + strlen(r);

;		// Reverse percent encoding
		if ((vallen = URIunescape(aival, MAX_AI_LEN, r, (size_t)(p-r))) == 0) {
			sprintf(ctx->errMsg, "Decoded AI (%.*s) from DL path info too long", (int)ailen, ai);
			goto fail;
		}

		// Special handling of AI (01) to pad up to a GTIN-14
		if (strcmp(entry->ai, "01") == 0 &&
		    (vallen == 13 || vallen == 12 || vallen == 8)) {
			for (i = 0; i <= 13; i++)
				aival[13-i] = vallen >= i+1 ? aival[vallen-i-1] : '0';
			aival[14] = '\0';
			vallen = 14;
		}

		DEBUG_PRINT("    Extracted: (%.*s) %.*s\n", ailen, ai, (int)vallen, aival);

		if (fnc1req)
			writeDataStr("#");			// Write FNC1, if required
		outai = dataStr + strlen(dataStr);		// Save start of AI for AI data
		nwriteDataStr(ai, ailen);			// Write AI
		fnc1req = gs1_isFNC1required(entry->ai);	// Record if required before next AI

		outval = dataStr + strlen(dataStr);		// Save start of value for AI data
		nwriteDataStr(aival, vallen);			// Write value

		// Perform certain checks at parse time, before processing the
		// components with the linters
		if (!gs1_aiValLengthContentCheck(ctx, entry, aival, vallen))
			goto fail;

		// Update the AI data
		if (ctx->numAIs < MAX_AIS) {
			ctx->aiData[ctx->numAIs].aiEntry = entry;
			ctx->aiData[ctx->numAIs].ai = outai;
			ctx->aiData[ctx->numAIs].ailen = (uint8_t)ailen;
			ctx->aiData[ctx->numAIs].value = outval;
			ctx->aiData[ctx->numAIs].vallen = (uint8_t)vallen;
			ctx->numAIs++;
		} else {
			strcpy(ctx->errMsg, "Too many AIs");
			goto fail;
		}
	}

	// Fragment character delimits end of the query parameters
	if (qp && ((fr = strchr(qp, '#')) != NULL))
		*fr++ = '\0';

	if (qp)
		DEBUG_PRINT("  Query params: %s\n", qp);

	p = qp;
	while (p && *p) {

		while (*p == '&')				// Jump any & separators
			p++;
		if ((r = strchr(p, '&')) == NULL)
			r = p + strlen(p);			// Value-pair finishes at end of data

		// Discard parameters with no value
		if ((e = memchr(p, '=', (size_t)(r-p))) == NULL) {
			DEBUG_PRINT("    Skipped singleton:   %.*s\n", (int)(r-p), p);
			p = r;
			continue;
		}

		// Numeric-only query parameters not matching an AI aren't allowed
		ai = p;
		ailen = (size_t)(e-p);
		entry = NULL;
		if (gs1_allDigits((uint8_t*)p, ailen) && (entry = gs1_lookupAIentry(ctx, p, ailen)) == NULL) {
			sprintf(ctx->errMsg, "Unknown AI (%.*s) in query parameters", (int)ailen, p);
			goto fail;
		}

		// Skip non-numeric query parameters
		if (!entry) {
			DEBUG_PRINT("    Skipped:   %.*s\n", (int)(r-p), p);
			p = r;
			continue;
		}

		// Reverse percent encoding
		e++;
		if ((vallen = URIunescape(aival, MAX_AI_LEN, e, (size_t)(r-e))) == 0) {
			sprintf(ctx->errMsg, "Decoded AI (%s) value from DL query params too long", entry->ai);
			goto fail;
		}

		// Special handling of AI (01) to pad up to a GTIN-14
		if (strcmp(entry->ai, "01") == 0 &&
		    (vallen == 13 || vallen == 12 || vallen == 8)) {
			for (i = 0; i <= 13; i++)
				aival[13-i] = vallen >= i+1 ? aival[vallen-i-1] : '0';
			aival[14] = '\0';
			vallen = 14;
		}

		DEBUG_PRINT("    Extracted: (%.*s) %.*s\n", ailen, ai, (int)vallen, aival);

		if (fnc1req)
			writeDataStr("#");			// Write FNC1, if required
		outai = dataStr + strlen(dataStr);		// Save start of AI for AI data
		nwriteDataStr(ai, ailen);			// Write AI
		fnc1req = gs1_isFNC1required(entry->ai);	// Record if required before next AI

		outval = dataStr + strlen(dataStr);		// Save start of value for AI data
		nwriteDataStr(aival, vallen);			// Write value

		// Perform certain checks at parse time, before processing the
		// components with the linters
		if (!gs1_aiValLengthContentCheck(ctx, entry, aival, vallen))
			goto fail;

		// Update the AI data
		if (ctx->numAIs < MAX_AIS) {
			ctx->aiData[ctx->numAIs].aiEntry = entry;
			ctx->aiData[ctx->numAIs].ai = outai;
			ctx->aiData[ctx->numAIs].ailen = (uint8_t)ailen;
			ctx->aiData[ctx->numAIs].value = outval;
			ctx->aiData[ctx->numAIs].vallen = (uint8_t)vallen;
			ctx->numAIs++;
		} else {
			strcpy(ctx->errMsg, "Too many AIs");
			goto fail;
		}

		p = r;

	}

	DEBUG_PRINT("Parsing DL data successful: %s\n", dataStr);

	// Now validate the data that we have written
	ret = gs1_processAIdata(ctx, dataStr, false);

out:

	if (qp)			// Restore original query parameter delimeter
		*(qp-1) = '?';

	if (fr)			// Restore original fragment delimieter
		*(fr-1) = '#';

	return ret;

fail:

	if (*ctx->errMsg == '\0')
		strcpy(ctx->errMsg, "Failed to parse DL data");
	ctx->errFlag = true;

	DEBUG_PRINT("Parsing DL data failed: %s\n", ctx->errMsg);

	*dataStr = '\0';
	ret = false;
	goto out;

}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"


static void test_parseDLuri(gs1_encoder *ctx, bool should_succeed, const char *dlData, const char* expect) {

	char in[256];
	char out[256];
	char casename[256];

	sprintf(casename, "%s => %s", dlData, expect);
	TEST_CASE(casename);

	ctx->numAIs = 0;
	strcpy(in, dlData);
	TEST_CHECK(gs1_parseDLuri(ctx, in, out) ^ !should_succeed);
	TEST_MSG("Err: %s", ctx->errMsg);
	if (should_succeed)
		TEST_CHECK(strcmp(out, expect) == 0);
	TEST_MSG("Given: %s; Got: %s; Expected: %s; Err: %s", dlData, out, expect, ctx->errMsg);

}


/*
 *  Convert a DL URI to a regular AI string "#..."
 *
 */
void test_dl_parseDLuri(void) {

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	test_parseDLuri(ctx, false,  "", "");
	test_parseDLuri(ctx, false,  "ftp://", "");
	test_parseDLuri(ctx, false,  "http://", "");
	test_parseDLuri(ctx, false,  "http:///", "");			// No domain
	test_parseDLuri(ctx, false,  "http://a", "");			// No path info
	test_parseDLuri(ctx, false,  "http://a/", "");			// Pathelogical minimal domain but no AI info

	test_parseDLuri(ctx, true,					// http
		"http://a/00/006141411234567890",
		"#00006141411234567890");

	test_parseDLuri(ctx, true,					// https
		"https://a/00/006141411234567890",
		"#00006141411234567890");

	test_parseDLuri(ctx, false,					// No domain
		"https://00/006141411234567890",
		"");

	test_parseDLuri(ctx, true,
		"https://a/stem/00/006141411234567890",
		"#00006141411234567890");

	test_parseDLuri(ctx, true,
		"https://a/more/stem/00/006141411234567890",
		"#00006141411234567890");

	test_parseDLuri(ctx, true,					// Fake AI in stem, stop at rightmost key
		"https://a/00/faux/00/006141411234567890",
		"#00006141411234567890");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333",
		"#0112312312312333");

	test_parseDLuri(ctx, true,					// GTIN-13 -> GTIN-14
		"https://a/01/2112345678900",
		"#0102112345678900");

	test_parseDLuri(ctx, true,					// GTIN-12 -> GTIN-14
		"https://a/01/416000336108",
		"#0100416000336108");

	test_parseDLuri(ctx, true,					// GTIN-8 -> GTIN-14
		"https://a/01/02345673",
		"#0100000002345673");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333/22/TEST/10/ABC/21/XYZ",
		"#011231231231233322TEST#10ABC#21XYZ");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333/235/TEST",
		"#0112312312312333235TEST");

	test_parseDLuri(ctx, true,
		"https://a/253/1231231231232",
		"#2531231231231232");

	test_parseDLuri(ctx, true,
		"https://a/253/1231231231232TEST5678901234567",
		"#2531231231231232TEST5678901234567");

	test_parseDLuri(ctx, false,
		"https://a/253/1231231231232TEST56789012345678", "");	// Too long N13 X0..17

	test_parseDLuri(ctx, true,
		"https://a/8018/123456789012345675/8019/123",
		"#8018123456789012345675#8019123");

	test_parseDLuri(ctx, false,
		"https://a/stem/00/006141411234567890/", ""); 		// Can't end in slash

	test_parseDLuri(ctx, true,
		"https://a/stem/00/006141411234567890?99=ABC",		// Query params; no FNC1 req after pathinfo
		 "#0000614141123456789099ABC");

	test_parseDLuri(ctx, true,
		"https://a/stem/401/12345678?99=ABC",			// Query params; FNC1 req after pathinfo
		 "#40112345678#99ABC");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333?99=ABC&98=XYZ",
		"#011231231231233399ABC#98XYZ");

	test_parseDLuri(ctx, false,
		"https://a/01/12312312312333?99=ABC&999=faux", "");	// Non-AI, numeric-only query param

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333?&&&99=ABC&&&&&&98=XYZ&&&",	// Extraneous query param separators
		"#011231231231233399ABC#98XYZ");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333?99=ABC&unknown=666&98=XYZ",
		"#011231231231233399ABC#98XYZ");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333?99=ABC&singleton&98=XYZ",
		"#011231231231233399ABC#98XYZ");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333?singleton&99=ABC&98=XYZ",
		"#011231231231233399ABC#98XYZ");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333?99=ABC&98=XYZ&singleton",
		"#011231231231233399ABC#98XYZ");

	test_parseDLuri(ctx, true,
		"https://a/01/12312312312333/22/ABC%2d123?99=ABC&98=XYZ%2f987",	// Percent escaped values
		"#011231231231233322ABC-123#99ABC#98XYZ/987");


	/*
	 * Examples for DL specification
	 *
	 */

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/09520123456788",
		"#0109520123456788");

	test_parseDLuri(ctx, true,
		"https://brand.example.com/01/9520123456788",
		"#0109520123456788");

	test_parseDLuri(ctx, true,
		"https://brand.example.com/some-extra/pathinfo/01/9520123456788",
		"#0109520123456788");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/09520123456788/22/2A",
		"#0109520123456788222A");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/09520123456788/10/ABC123",
		"#010952012345678810ABC123");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/09520123456788/21/12345",
		"#01095201234567882112345");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/09520123456788/10/ABC1/21/12345?17=180426",
		"#010952012345678810ABC1#2112345#17180426");
	// Specification sorts (17) before (10) and (21):
	//   "#01095201234567881718042610ABC1#2112345"

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/09520123456788?3103=000195",
		"#01095201234567883103000195");

	test_parseDLuri(ctx, true,
		"https://example.com/01/9520123456788?3103=000195&3922=0299&17=201225",
		"#0109520123456788310300019539220299#17201225");

	test_parseDLuri(ctx, true,
		"https://example.com/01/9520123456788?3103=000195&3922=0299&17=201225",
		"#0109520123456788310300019539220299#17201225");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/9520123456788?3103=000195&3922=0299&17=201225",
		"#0109520123456788310300019539220299#17201225");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/01/9520123456788?17=201225&3103=000195&3922=0299",
		"#010952012345678817201225310300019539220299");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/00/952012345678912345",
		"#00952012345678912345");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/00/952012345678912345?02=09520123456788&37=25&10=ABC123",
		"#0095201234567891234502095201234567883725#10ABC123");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/414/9520123456788",
		"#4149520123456788");

	test_parseDLuri(ctx, true,
		"https://id.gs1.org/414/9520123456788/254/32a%2Fb",
		"#414952012345678825432a/b");

	test_parseDLuri(ctx, true,
		"https://example.com/8004/9520614141234567?01=9520123456788",
		"#80049520614141234567#0109520123456788");


	// Examples with unknown AIs, not permitted
	test_parseDLuri(ctx, false,
		"https://example.com/01/9520123456788/89/ABC123?99=XYZ",
		"");

	test_parseDLuri(ctx, false,
		"https://example.com/01/9520123456788?99=XYZ&89=ABC123",
		"");

	// Examples with unknown AIs, permitted
	gs1_encoder_setPermitUnknownAIs(ctx, true);

	test_parseDLuri(ctx, true,
		"https://example.com/01/9520123456788/89/ABC123?99=XYZ",
		"#010952012345678889ABC123#99XYZ");

	test_parseDLuri(ctx, true,
		"https://example.com/01/9520123456788?99=XYZ&89=ABC123",
		"#010952012345678899XYZ#89ABC123");

	gs1_encoder_free(ctx);

}


static void test_URIunescape(const char *in, const char *expect) {

	char out[MAX_AI_LEN+1];

	TEST_CHECK(URIunescape(out, MAX_AI_LEN, in, strlen(in)) == strlen(expect));
	TEST_CHECK(strcmp(out, expect) == 0);
	TEST_MSG("Given: %s; Got: %s; Expected: %s", in, out, expect);

}


void test_dl_URIunescape(void) {

	char out[MAX_AI_LEN+1];

	test_URIunescape("", "");
	test_URIunescape("test", "test");
	test_URIunescape("%20", " ");
	test_URIunescape("%20AB", " AB");
	test_URIunescape("A%20B", "A B");
	test_URIunescape("AB%20", "AB ");
	test_URIunescape("ABC%2", "ABC%2");			// Off end
	test_URIunescape("ABCD%", "ABCD%");
	test_URIunescape("A%20%20B", "A  B");			// Run together
	test_URIunescape("A%01B", "A" "\x01" "B");		// "Minima", we check \0 below
	test_URIunescape("A%ffB", "A" "\xFF" "B");		// Maxima
	test_URIunescape("A%FfB", "A" "\xFF" "B");		// Case mixing
	test_URIunescape("A%fFB", "A" "\xFF" "B");		// Case mixing
	test_URIunescape("A%FFB", "A" "\xFF" "B");		// Case mixing
	test_URIunescape("A%4FB", "AOB");
	test_URIunescape("A%4fB", "AOB");
	test_URIunescape("A%4gB", "A%4gB");			// Non hex digit
	test_URIunescape("A%4GB", "A%4GB");			// Non hex digit
	test_URIunescape("A%g4B", "A%g4B");			// Non hex digit
	test_URIunescape("A%G4B", "A%G4B");			// Non hex digit

	// Check that \0 is sane, although we are only working with strings
	TEST_CHECK(URIunescape(out, MAX_AI_LEN, "A%00B", 5) == 3);
	TEST_CHECK(memcmp(out, "A" "\x00" "B", 4) == 0);

	// Truncated input
	TEST_CHECK(URIunescape(out, MAX_AI_LEN, "ABCD", 2) == 2);
	TEST_CHECK(memcmp(out, "AB", 3) == 0);			// Includes \0

	// Truncated output
	TEST_CHECK(URIunescape(out, 2, "ABCD", 4) == 2);
	TEST_CHECK(memcmp(out, "AB", 3) == 0);			// Includes \0

}


#endif  /* UNIT_TESTS */

