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
#include <string.h>

#include "gs1encoders.h"
#include "gs1.h"


// AI prefixes that are defined as not requiring termination by an FNC1 character
static const char* fixedAIprefixes[22] = {
	"00", "01", "02",
	"03", "04",
	"11", "12", "13", "14", "15", "16", "17", "18", "19",
	"20",
	// "23",	// No longer defined as fixed length
	"31", "32", "33", "34", "35", "36",
	"41"
};


// Write to dataStr checking for overflow
#define writeDataStr(v) do {						\
	if (strlen(dataStr) + strlen(v) > GS1_ENCODERS_MAX_DATA)	\
		goto fail;						\
	strcat(dataStr, v);						\
} while (0)

#define nwriteDataStr(v,l) do {						\
	if (strlen(dataStr) + l > GS1_ENCODERS_MAX_DATA)		\
		goto fail;						\
	strncat(dataStr, v, l);						\
} while (0)


// Convert GS1 AI syntax data to regular data string with # = FNC1
bool gs1_parseGS1data(char *gs1Data, char *dataStr) {

	assert(gs1Data);
	assert(dataStr);

	char *p = gs1Data;
	char *r;
	int i;
	bool fnc1req = true;

	*dataStr = '\0';

	while (*p) {

		if (*p++ != '(') goto fail; 			// Expect start of AI
		if (!(r = strchr(p, ')'))) goto fail;		// Find end of A
		if (r-p < 2 || r-p > 4) goto fail;		// AI is 2-4 characters
		*r++ = '\0';					// Delimit the end of the AI pointed to by p
		if (!gs1_allDigits((uint8_t*)p)) goto fail;	// AI must be numeric

		if (fnc1req)
			writeDataStr("#");			// Write FNC1, if required
		writeDataStr(p);				// Write AI

		fnc1req = true;					// Determine whether FNC1 required before next AI
		for (i = 0; i < (int)(sizeof(fixedAIprefixes) / sizeof(fixedAIprefixes[0])); i++)
			if (strncmp(fixedAIprefixes[i], p, 2) == 0)
				fnc1req = false;

		if (!*r)					// Fail if message ends after AI and no value
			goto fail;

again:

		if ((p = strchr(r, '(')) == NULL) {
			writeDataStr(r);			// Write until end of data
			break;
		}

		if (*(p-1) == '\\') {				// This bracket is an escaped data character
			nwriteDataStr(r, (size_t)(p-r-1));	// Write up to the escape character
			writeDataStr("(");			// Write the data bracket
			r = p+1;				// And keep going
			goto again;
		}

		if (p-r < 1) goto fail;				// Value cannot be empty
		nwriteDataStr(r, (size_t)(p-r));		// Write the value

	}

	return true;

fail:

	*dataStr = '\0';
	return false;

}


// Validate and set the parity digit
bool gs1_validateParity(uint8_t *str) {

	int weight;
	int parity = 0;

	assert(*str);

	weight = strlen((char*)str) % 2 == 0 ? 3 : 1;
	while (*(str+1)) {
		parity += weight * (*str++ - '0');
		weight = 4 - weight;
	}
	parity = (10 - parity%10) % 10;

	if (parity + '0' == *str) return true;

	*str = (uint8_t)(parity + '0');		// Recalculate
	return false;

}


bool gs1_allDigits(uint8_t *str) {

	assert(str);

	while (*str) {
		if (*str < '0' || *str >'9')
			return false;
		str++;
	}
	return true;

}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"


static void test_parseGS1data(bool should_succeed, char *gs1data, char* expect) {

	char in[256];
	char out[256];
	char casename[256];

	sprintf(casename, "%s => %s", gs1data, expect);
	TEST_CASE(casename);

	strcpy(in, gs1data);
	TEST_CHECK(gs1_parseGS1data(in, out) ^ !should_succeed);
	if (should_succeed)
		TEST_CHECK(strcmp(out, expect) == 0);
	TEST_MSG("Given: %s; Got: %s; Expected: %s", gs1data, out, expect);

}


void test_gs1_parseGS1data(void) {

	test_parseGS1data(true,  "(01)12345678901231", "#0112345678901231");
	test_parseGS1data(true,  "(10)12345", "#1012345");
	test_parseGS1data(true,  "(01)12345678901231(10)12345", "#01123456789012311012345");	// No FNC1 after (01)
	test_parseGS1data(true,  "(3199)12345(10)12345", "#3199123451012345");			// No FNC1 after (3199)
	test_parseGS1data(true,  "(10)12345(11)98765", "#1012345#1198765");			// FNC1 after (10)
	test_parseGS1data(true,  "(3799)12345(11)98765", "#379912345#1198765");			// FNC1 after (3799)
	test_parseGS1data(true,  "(10)12345\\(11)98765", "#1012345(11)98765");			// Escaped bracket
	test_parseGS1data(true,  "(10)12345\\(", "#1012345(");					// At end if fine

	test_parseGS1data(false, "(10)(11)98765", "");						// Value must not be empty
	test_parseGS1data(false, "(10)12345(11)", "");						// Value must not be empty
	test_parseGS1data(false, "(1A)12345", "");						// AI must be numeric
	test_parseGS1data(false, "1(12345", "");						// Must start with AI
	test_parseGS1data(false, "12345", "");							// Must start with AI
	test_parseGS1data(false, "()12345", "");						// AI too short
	test_parseGS1data(false, "(1)12345", "");						// AI too short
	test_parseGS1data(false, "(12345)12345", "");						// AI too long
	test_parseGS1data(false, "(15", "");							// AI must terminate
	test_parseGS1data(false, "(1", "");							// AI must terminate
	test_parseGS1data(false, "(", "");							// AI must terminate

	test_parseGS1data(true,  "(01)12345678901231|(10)12345", "#0112345678901231|1012345");	// Allow linear/CC separation
	test_parseGS1data(true,  "(10)12345|(11)98765", "#1012345|#1198765");			// Leading "#" is optional in CC

}


void test_gs1_validateParity(void) {

	char good_gtin14[] = "24012345678905";
	char bad_gtin14[]  = "24012345678909";
	char good_gtin13[] = "2112233789657";
	char bad_gtin13[]  = "2112233789658";
	char good_gtin12[] = "416000336108";
	char bad_gtin12[]  = "416000336107";
	char good_gtin8[]  = "02345680";
	char bad_gtin8[]   = "02345689";

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin14));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin14));
	TEST_CHECK(bad_gtin14[13] == '5');		// Recomputed

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin13));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin13));
	TEST_CHECK(bad_gtin13[12] == '7');		// Recomputed

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin12));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin12));
	TEST_CHECK(bad_gtin12[11] == '8');		// Recomputed

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin8));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin8));
	TEST_CHECK(bad_gtin8[7] == '0');		// Recomputed

}


#endif  /* UNIT_TESTS */

