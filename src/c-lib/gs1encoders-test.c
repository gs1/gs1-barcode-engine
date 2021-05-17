/**
 * GS1 barcode encoder application
 *
 * @author Copyright (c) 2000-2020 GS1 AISBL.
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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-folding-constant"
#endif
#include "acutest.h"
#if defined(__clang__)
#pragma clang diagnostic push
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "enc-private.h"
#include "gs1encoders.h"
#include "cc.h"
#include "ucc128.h"


void test_print_strings(gs1_encoder *ctx) {

	char **strings;

	gs1_encoder_getBufferStrings(ctx, &strings);

	printf("\n\n");

	int i = 0;
	while (strings[i]) {
		printf("%s\n", strings[i++]);
	}

	printf("\n");

}


bool test_encode(gs1_encoder *ctx, char* dataStr, char** expect) {

	char **strings;
	char *tail;
	int i = 0;

	TEST_CHECK(gs1_encoder_setDataStr(ctx, dataStr));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK(gs1_encoder_getBufferStrings(ctx, &strings) > 0);

	while (strings[i] && expect[i] &&
			strcmp(expect[i], "...") != 0 &&
			strcmp(expect[i], "vvv") != 0) {
		if (strcmp(strings[i], expect[i]) != 0)
			return false;
		i++;
	}

	if (!expect[i] && !strings[i])
		return true;   // Matched to the end

	if (!strings[i])
		return false;  // Symbol too short

	if (expect[i] && strcmp(expect[i], "...") == 0)
		return true;   // Matched all provided

	if (!expect[i] || strcmp(expect[i], "vvv") != 0)
		return false;

	// "vvv" provided so we expect same rows all the way down
	tail = expect[i-1];
	while (strings[i]) {
		if (strcmp(strings[i], tail) != 0)
			return false;
		i++;
	}

	return true;

}


TEST_LIST = {

    /*
     * gs1encoders.c
     *
     */
    { "api_getVersion", test_api_getVersion },
    { "api_defaults", test_api_defaults },
    { "api_sym", test_api_sym },
    { "api_fileInputFlag", test_api_fileInputFlag },
    { "api_pixMult", test_api_pixMult },
    { "api_XYundercut", test_api_XYundercut },
    { "api_sepHt", test_api_sepHt },
    { "api_segWidth", test_api_segWidth },
    { "api_linHeight", test_api_linHeight },
    { "api_outFile", test_api_outFile },
    { "api_dataFile", test_api_dataFile },
    { "api_dataStr", test_api_dataStr },
    { "api_format", test_api_format },
    { "api_getBuffer", test_api_getBuffer },


    /*
     * cc.c
     *
     */
    { "cc_encode928", test_cc_encode928 },


    /*
     * ucc128.c
     *
     */
    { "ucc128_encode", test_ucc128_encode },

    { NULL, NULL }
};
