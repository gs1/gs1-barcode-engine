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

#include "gs1.h"


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

