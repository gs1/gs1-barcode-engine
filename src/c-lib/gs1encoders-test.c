/**
 * GS1 Barcode Engine
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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-folding-constant"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#elif defined(_MSC_VER)
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#endif
#include "acutest.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "enc-private.h"
#include "gs1encoders.h"
#include "cc.h"
#include "dm.h"
#include "ean.h"
#include "ai.h"
#include "dl.h"
#include "qr.h"
#include "rss14.h"
#include "rssexp.h"
#include "rsslim.h"
#include "scandata.h"
#include "ucc128.h"


void test_print_codewords(const uint8_t *cws, const int numcws) {
	int i;
	for (i=0; i < numcws; i++)
		printf("%d ", cws[i]);
	printf("\n");
}

void test_print_bits(const uint8_t *bytes, const int numbits) {
	int i;
	for (i=0; i < numbits; i++)
		printf("%d", bytes[i/8] >> (7-i%8) & 1);
	printf("\n");
}


void test_print_strings(gs1_encoder *ctx) {

	char **strings;
	int i;

	gs1_encoder_getBufferStrings(ctx, &strings);

	assert(strings);

	printf("\n\n");

	i = 0;
	while (strings[i]) {
		printf("%s\n", strings[i++]);
	}

	printf("\n");

}


bool test_encode(gs1_encoder *ctx, const bool should_succeed, const int sym, const char* dataStr, const char** expect) {

	char **strings;
	const char *tail;
	int i = 0;

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dRAW));
	TEST_CHECK(gs1_encoder_setSym(ctx, sym));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, dataStr));
	TEST_MSG("Error: %s", gs1_encoder_getErrMsg(ctx));

	TEST_CHECK(gs1_encoder_encode(ctx) ^ !should_succeed);
	TEST_MSG("Error: %s", gs1_encoder_getErrMsg(ctx));
	if (!should_succeed)
		return true;

	TEST_ASSERT(gs1_encoder_getBufferStrings(ctx, &strings) > 0);

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
    { "api_instanceSize", test_api_instanceSize },
    { "api_maxUcc128LinHeight", test_api_maxUcc128LinHeight },
    { "api_maxFilenameLength", test_api_maxFilenameLength },
    { "api_maxInputBuffer", test_api_maxInputBuffer },
    { "api_maxPixMult", test_api_maxPixMult },
    { "api_defaults", test_api_defaults },
    { "api_sym", test_api_sym },
    { "api_fileInputFlag", test_api_fileInputFlag },
    { "api_pixMult", test_api_pixMult },
    { "api_Xdimension", test_api_Xdimension },
    { "api_XYundercut", test_api_XYundercut },
    { "api_sepHt", test_api_sepHt },
    { "api_segWidth", test_api_segWidth },
    { "api_linHeight", test_api_linHeight },
    { "api_dmRowsColumns", test_api_dmRowsColumns },
    { "api_qrVersion", test_api_qrVersion },
    { "api_qrEClevel", test_api_qrEClevel },
    { "api_addCheckDigit", test_api_addCheckDigit },
    { "api_permitUnknownAIs", test_api_permitUnknownAIs },
    { "api_outFile", test_api_outFile },
    { "api_dataFile", test_api_dataFile },
    { "api_dataStr", test_api_dataStr },
    { "api_getAIdataStr", test_api_getAIdataStr },
    { "api_getScanData", test_api_getScanData },
    { "api_setScanData", test_api_setScanData },
    { "api_getHRI", test_api_getHRI },
    { "api_format", test_api_format },
    { "api_getBuffer", test_api_getBuffer },
    { "api_copyOutputBuffer", test_api_copyOutputBuffer },
    { "api_copyHRI", test_api_copyHRI },


    /*
     * ai.c
     *
     */
    { "ai_lookupAIentry", test_ai_lookupAIentry },
    { "ai_AItableVsPrefixLength", test_ai_AItableVsPrefixLength },
    { "ai_gs1_parseAIdata", test_ai_parseAIdata },
    { "ai_gs1_processAIdata", test_ai_processAIdata },
    { "ai_validateParity", test_ai_validateParity },
    { "ai_lint_csumalpha", test_ai_lint_csumalpha },


    /*
     * dl.c
     *
     */
    { "dl_gs1_parseDLuri", test_dl_parseDLuri },
    { "dl_URIunescape", test_dl_URIunescape },


    /*
     * scandata.c
     *
     */
    { "scandata_generateScanData", test_scandata_generateScanData },
    { "scandata_processScanData", test_scandata_processScanData },


    /*
     * cc.c
     *
     */
    { "cc_encode928", test_cc_encode928 },


    /*
     * dm.c
     *
     */
    { "dm_DM_encode", test_dm_DM_encode },


    /*
     * ean.c
     *
     */
    { "ean_EAN13_encode_ean13", test_ean_EAN13_encode_ean13 },
    { "ean_EAN13_encode_upca", test_ean_EAN13_encode_upca },
    { "ean_EAN8_encode", test_ean_EAN8_encode },
    { "ean_UPCA_encode", test_ean_UPCA_encode },
    { "ean_UPCE_encode", test_ean_UPCE_encode },
    { "ean_zeroCompress", test_ean_zeroCompress },


    /*
     * rss.c
     *
     */
    { "rss14_RSS14_encode", test_rss14_RSS14_encode },
    { "rss14_RSS14T_encode", test_rss14_RSS14T_encode },
    { "rss14_RSS14S_encode", test_rss14_RSS14S_encode },
    { "rss14_RSS14SO_encode", test_rss14_RSS14SO_encode },


    /*
     * rsslim.c
     *
     */
    { "rsslim_RSSLIM_encode", test_rsslim_RSSLIM_encode },


    /*
     * rssexp.c
     *
     */
    { "rssexp_RSSEXP_encode", test_rssexp_RSSEXP_encode },


    /*
     * qr.c
     *
     */
#ifdef SLOW_TESTS
    { "qr_QR_versions", test_qr_QR_versions },
#endif
    { "qr_QR_fixtures", test_qr_QR_fixtures },
    { "qr_QR_encode", test_qr_QR_encode },


    /*
     * ucc128.c
     *
     */
    { "ucc_UCC128A_encode", test_ucc_UCC128A_encode },
    { "ucc_UCC128C_encode", test_ucc_UCC128C_encode },


    { NULL, NULL }
};
