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
#include <stdio.h>
#include <string.h>

#include "enc-private.h"
#include "gs1encoders.h"
#include "ean.h"
#include "gs1.h"
#include "rss14.h"
#include "rsslim.h"


static void scancat(char* out, const char* in) {

	const char *p;
	char *q;

	p = in;
	q = out;

	while (*q)
		q++;					// Got to end of output

	if (*p == '#') {				// GS1 mode
		p++;					// Skip the leading FNC1 since we are following a symbology identifier
		while (*p) {
			if (*p == '#')			// Convert encoded FNC1 to GS
				*q++ = '\x1D';
			else
				*q++ = *p;
			p++;
		}
		if (*(p - 1) == '#')			// Strip any trailing FNC1
			q--;
	}
	else {
		if (strlen(p) >= 2 && strncmp(p, "\\#", 2) == 0) {		// "\#" -> "#..."
			p++;
		}
		else if (strlen(p) >= 3 && strncmp(p, "\\\\#", 3) == 0) {	// "\\#" -> "\#..."
			p++;
		}
		while (*p)
			*q++ = *p++;
	}
	*q = '\0';

}


char* gs1_generateScanData(gs1_encoder* ctx) {

	char* cc = NULL;
	int i;
	bool lastAIfnc1;
	char *prefix;
	char primaryStr[15];
	char* ret;

	assert(ctx);

	*ctx->outStr = '\0';

	if (*ctx->dataStr == '\0')
		return ctx->outStr;

	if ((cc = strchr(ctx->dataStr, '|')) != NULL)		// Delimit end of linear data
		*cc++ = '\0';

	switch (ctx->sym) {

	case gs1_encoder_sQR:
		// "]Q1" for plain data; "]Q3" for GS1 data
		strcat(ctx->outStr, *ctx->dataStr == '#' ? "]Q3" : "]Q1");
		scancat(ctx->outStr, ctx->dataStr);
		break;

	case gs1_encoder_sDM:
		// "]d1" for plain data; "]d2" for GS1 data
		strcat(ctx->outStr, *ctx->dataStr == '#' ? "]d2" : "]d1");
		scancat(ctx->outStr, ctx->dataStr);
		break;

	case gs1_encoder_sGS1_128_CCA:
	case gs1_encoder_sGS1_128_CCC:

		if (!cc) {
			// "]C1" for linear-only GS1-128
			assert(*ctx->dataStr == '#');
			strcat(ctx->outStr, "]C1");
			scancat(ctx->outStr, ctx->dataStr);
			break;
		}

		/* FALLTHROUGH */  // For GS1-128 Composite

	case gs1_encoder_sDataBarExpanded:

		// "]e0" followed by concatenated AI data from linear and CC

		assert(*ctx->dataStr == '#');
		strcat(ctx->outStr, "]e0");
		scancat(ctx->outStr, ctx->dataStr);

		if (cc) {
			assert(*cc == '#');

			// Append GS if last AI of linear component isn't fixed-length
			lastAIfnc1 = false;
			for (i = 0; i < ctx->numAIs && ctx->aiData[i].aiEntry; i++)
				lastAIfnc1 = ctx->aiData[i].aiEntry->fnc1;
			if (lastAIfnc1)
				strcat(ctx->outStr, "\x1D");

			scancat(ctx->outStr, cc);
		}

		break;

	case gs1_encoder_sDataBarOmni:
	case gs1_encoder_sDataBarTruncated:
	case gs1_encoder_sDataBarStacked:
	case gs1_encoder_sDataBarStackedOmni:
	case gs1_encoder_sDataBarLimited:

		// "]e0" followed by concatenated AI data from linear and CC

		// Normalise input to 14 digits
		if (ctx->sym == gs1_encoder_sDataBarLimited)
			gs1_normaliseRSSLim(ctx, ctx->dataStr, primaryStr);
		else
			gs1_normaliseRSS14(ctx, ctx->dataStr, primaryStr);

		if (*primaryStr == '\0')
			goto fail;

		strcat(ctx->outStr, "]e001");		// Convert to AI (01)
		scancat(ctx->outStr, primaryStr);

		if (cc) {
			assert(*cc == '#');
			scancat(ctx->outStr, cc);
		}

		break;

	case gs1_encoder_sUPCA:
	case gs1_encoder_sUPCE:
	case gs1_encoder_sEAN13:
	case gs1_encoder_sEAN8:

		// Primary is "]E0" then 13 digits (or "]E4" then 8 digits for EAN-8)
		// CC is new message beginning "]e0"

		// Normalise input
		if (ctx->sym == gs1_encoder_sEAN8) {
			gs1_normaliseEAN8(ctx, ctx->dataStr, primaryStr);
			prefix = "]E4";
		}
		else if (ctx->sym == gs1_encoder_sUPCE) {
			gs1_normaliseUPCE(ctx, ctx->dataStr, primaryStr);
			prefix = "]E00";	// UPCE is normalised to 12 digits
		}
		else {	// EAN13 and UPC-A
			gs1_normaliseEAN13(ctx, ctx->dataStr, primaryStr);
			prefix = "]E0";
		}

		if (*primaryStr == '\0')
			goto fail;

		strcat(ctx->outStr, prefix);
		scancat(ctx->outStr, primaryStr);
		if (cc) {
			assert(*cc == '#');
			strcat(ctx->outStr, "|]e0");		// "|" means start of new message
			scancat(ctx->outStr, cc);
		}
		break;

	}

	ret = ctx->outStr;

out:

	if (cc)
		*(cc - 1) = '|';			// Put original separator back
	return ret;

fail:

	ret = NULL;
	goto out;

}


bool gs1_processScanData(gs1_encoder* ctx, char* scanData) {

	assert(ctx);
	assert(scanData);

	return true;

}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"


#define test_testGenerateScanData(c, n, d, e) do {					\
	do_test_testGenerateScanData(c, #n, gs1_encoder_s##n, d, e);			\
} while (0)

static void do_test_testGenerateScanData(gs1_encoder* ctx, char* name, int sym, char* dataStr, char* expect) {

	char in[256];
	char *out;
	char casename[256];

	sprintf(casename, "%s: %s", name, dataStr);
	TEST_CASE(casename);

	strcpy(in, dataStr);
	TEST_ASSERT(gs1_encoder_setSym(ctx, sym));
	TEST_ASSERT(gs1_encoder_setDataStr(ctx, dataStr));
	TEST_ASSERT((out = gs1_encoder_getScanData(ctx)) != NULL);
	TEST_CHECK(strcmp(out, expect) == 0);
	TEST_MSG("Given: %s; Got: %s; Expected: %s", dataStr, out, expect);

}


void test_scandata_generateScanData(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	test_testGenerateScanData(ctx, NONE, "", "");
	test_testGenerateScanData(ctx, NONE, "TESTING", "");

	/* QR */
	test_testGenerateScanData(ctx, QR, "TESTING", "]Q0TESTING");
	test_testGenerateScanData(ctx, QR, "\\#TESTING", "]Q0#TESTING");		// Escaped data "#" character
	test_testGenerateScanData(ctx, QR, "\\\\#TESTING", "]Q0\\#TESTING");		// Escaped data "\#" characters
	test_testGenerateScanData(ctx, QR, "#011231231231233310ABC123#99TESTING",
		"]Q3011231231231233310ABC123" "\x1D" "99TESTING");

	/* DM */
	test_testGenerateScanData(ctx, DM, "TESTING", "]d1TESTING");
	test_testGenerateScanData(ctx, DM, "\\#TESTING", "]d1#TESTING");		// Escaped data "#" character
	test_testGenerateScanData(ctx, DM, "\\\\#TESTING", "]d1\\#TESTING");		// Escaped data "\#" characters
	test_testGenerateScanData(ctx, DM, "#011231231231233310ABC123#99TESTING",
		"]d2011231231231233310ABC123" "\x1D" "99TESTING");
	test_testGenerateScanData(ctx, DM, "#011231231231233310ABC123#99TESTING#",
		"]d2011231231231233310ABC123" "\x1D" "99TESTING");			// Trailing FNC1 should be stripped

	/* DataBar Expanded */
	test_testGenerateScanData(ctx, DataBarExpanded, "#011231231231233310ABC123#99TESTING",
		"]e0011231231231233310ABC123" "\x1D" "99TESTING");
	test_testGenerateScanData(ctx, DataBarExpanded, 				// ... Variable-length AI | Composite
		"#011231231231233310ABC123#99TESTING|#98COMPOSITE#97XYZ",
		"]e0011231231231233310ABC123" "\x1D" "99TESTING" "\x1D" "98COMPOSITE" "\x1D" "97XYZ");
	test_testGenerateScanData(ctx, DataBarExpanded,					// ... Fixed-Length AI | Composite
		"#011231231231233310ABC123#11991225|#98COMPOSITE#97XYZ",
		"]e0011231231231233310ABC123" "\x1D" "1199122598COMPOSITE" "\x1D" "97XYZ");

	/* GS1-128 */
	test_testGenerateScanData(ctx, GS1_128_CCA, "#011231231231233310ABC123#99TESTING",
		"]C1011231231231233310ABC123" "\x1D" "99TESTING");
	test_testGenerateScanData(ctx, GS1_128_CCA,					// Composite uses ]e0
		"#011231231231233310ABC123#99TESTING|#98COMPOSITE#97XYZ",
		"]e0011231231231233310ABC123" "\x1D" "99TESTING" "\x1D" "98COMPOSITE" "\x1D" "97XYZ");

	/* DataBar OmniDirectional */
	test_testGenerateScanData(ctx, DataBarOmni, "#0124012345678905|#99COMPOSITE#98XYZ",
		"]e0012401234567890599COMPOSITE" "\x1D" "98XYZ");
	test_testGenerateScanData(ctx, DataBarOmni, "24012345678905|#99COMPOSITE#98XYZ",
		"]e0012401234567890599COMPOSITE" "\x1D" "98XYZ");

	/* DataBar Limited */
	test_testGenerateScanData(ctx, DataBarLimited, "#0115012345678907|#99COMPOSITE#98XYZ",
		"]e0011501234567890799COMPOSITE" "\x1D" "98XYZ");
	test_testGenerateScanData(ctx, DataBarLimited, "15012345678907|#99COMPOSITE#98XYZ",
		"]e0011501234567890799COMPOSITE" "\x1D" "98XYZ");

	/* UPC-A */
	test_testGenerateScanData(ctx, UPCA, "#0100416000336108|#99COMPOSITE#98XYZ",
		"]E00416000336108|]e099COMPOSITE" "\x1D" "98XYZ");
	test_testGenerateScanData(ctx, UPCA, "416000336108|#99COMPOSITE#98XYZ",
		"]E00416000336108|]e099COMPOSITE" "\x1D" "98XYZ");

	/* UPC-E */
	test_testGenerateScanData(ctx, UPCE, "#0100001234000057|#99COMPOSITE#98XYZ",
		"]E00001234000057|]e099COMPOSITE" "\x1D" "98XYZ");
	test_testGenerateScanData(ctx, UPCE, "001234000057|#99COMPOSITE#98XYZ",
		"]E00001234000057|]e099COMPOSITE" "\x1D" "98XYZ");

	/* EAN-13 */
	test_testGenerateScanData(ctx, EAN13, "#0102112345678900|#99COMPOSITE#98XYZ",
		"]E02112345678900|]e099COMPOSITE" "\x1D" "98XYZ");
	test_testGenerateScanData(ctx, EAN13, "2112345678900|#99COMPOSITE#98XYZ",
		"]E02112345678900|]e099COMPOSITE" "\x1D" "98XYZ");

	/* EAN-8 */
	test_testGenerateScanData(ctx, EAN8, "#0100000002345673|#99COMPOSITE#98XYZ",
		"]E402345673|]e099COMPOSITE" "\x1D" "98XYZ");
	test_testGenerateScanData(ctx, EAN8, "02345673|#99COMPOSITE#98XYZ",
		"]E402345673|]e099COMPOSITE" "\x1D" "98XYZ");

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
