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


#include <stdbool.h>
#include <string.h>

#include "acutest.h"

#include "driver.h"
#include "gs1encoders.h"


static void test_getVersion() {

	gs1_encoder* ctx;
	char* version;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	version = gs1_encoder_getVersion(ctx);
	TEST_CHECK(version != NULL && strlen(version) > 0);

	gs1_encoder_free(ctx);

}


static void test_defaults() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_getPixMult(ctx) == 1);
	TEST_CHECK(gs1_encoder_getXundercut(ctx) == 0);
	TEST_CHECK(gs1_encoder_getYundercut(ctx) == 0);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 1);
	TEST_CHECK(gs1_encoder_getSegWidth(ctx) == 22);
	TEST_CHECK(gs1_encoder_getLinHeight(ctx) == 25);
	TEST_CHECK(gs1_encoder_getBmp(ctx) == false);    // tiff
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);
	TEST_CHECK(gs1_encoder_getFileInputFlag(ctx) == false);    // dataStr
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "") == 0);
	TEST_CHECK(strcmp(gs1_encoder_getDataFile(ctx), "data.txt") == 0);

	gs1_encoder_free(ctx);

}


static void test_sym() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sRSS14));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sRSS14T));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sRSS14S));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sRSS14SO));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sRSSLIM));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sRSSEXP));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUPCA));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUPCE));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN13));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN8));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUCC128_CCA));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUCC128_CCC));

	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sNONE));          // First
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sNUMSYMS - 1));   // Last
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNUMSYMS - 1);
	TEST_CHECK(!gs1_encoder_setSym(ctx, gs1_encoder_sNONE - 1));     // Too small
	TEST_CHECK(!gs1_encoder_setSym(ctx, gs1_encoder_sNUMSYMS));      // Too big
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUPCA));
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sUPCA);

	TEST_CHECK(gs1_encoder_sNUMSYMS == 12);  // Remember to add new symbologies

	gs1_encoder_free(ctx);

}


static void test_fileInputFlag() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setFileInputFlag(ctx, true));     // dataFile
	TEST_CHECK(gs1_encoder_getFileInputFlag(ctx) == true);

	gs1_encoder_free(ctx);

}


static void test_pixMult() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setPixMult(ctx, 1));
	TEST_CHECK(gs1_encoder_getPixMult(ctx) == 1);
	TEST_CHECK(!gs1_encoder_setPixMult(ctx, 0));
	TEST_CHECK(!gs1_encoder_setPixMult(ctx, GS1_ENCODERS_MAX_PIXMULT + 1));
	TEST_CHECK(gs1_encoder_setPixMult(ctx, 2));
	TEST_CHECK(gs1_encoder_getPixMult(ctx) == 2);

	// Check X/Y undercut are reset under appropriate conditions
	gs1_encoder_setPixMult(ctx, 4);
	gs1_encoder_setXundercut(ctx, 1);
	gs1_encoder_setYundercut(ctx, 1);
	TEST_CHECK(gs1_encoder_getXundercut(ctx) == 1);
	TEST_CHECK(gs1_encoder_getYundercut(ctx) == 1);
	gs1_encoder_setPixMult(ctx, 1);
	TEST_CHECK(gs1_encoder_getXundercut(ctx) == 0);
	TEST_CHECK(gs1_encoder_getYundercut(ctx) == 0);

	// But not reset otherwise
	gs1_encoder_setPixMult(ctx, 4);
	gs1_encoder_setXundercut(ctx, 1);
	gs1_encoder_setYundercut(ctx, 1);
	TEST_CHECK(gs1_encoder_getXundercut(ctx) == 1);
	TEST_CHECK(gs1_encoder_getYundercut(ctx) == 1);
	gs1_encoder_setPixMult(ctx, 3);
	TEST_CHECK(gs1_encoder_getXundercut(ctx) == 1);
	TEST_CHECK(gs1_encoder_getYundercut(ctx) == 1);

	// Check sepHt is reset under appropriate conditions
	gs1_encoder_setPixMult(ctx, 4);
	gs1_encoder_setSepHt(ctx, 5);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 5);
	gs1_encoder_setPixMult(ctx, 5);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 5);  // still valid
	gs1_encoder_setPixMult(ctx, 6);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 6);  // must update

	gs1_encoder_setPixMult(ctx, 4);
	gs1_encoder_setSepHt(ctx, 6);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 6);
	gs1_encoder_setPixMult(ctx, 3);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 6);  // still valid
	gs1_encoder_setPixMult(ctx, 2);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 2);  // must update

	gs1_encoder_free(ctx);

}


static void test_XYundercut() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	// Special case
	gs1_encoder_setPixMult(ctx, 1);
	TEST_CHECK(gs1_encoder_setXundercut(ctx, 0));
	TEST_CHECK(gs1_encoder_setYundercut(ctx, 0));

	// Minima
	gs1_encoder_setPixMult(ctx, 2);
	TEST_CHECK(gs1_encoder_setXundercut(ctx, 1));
	TEST_CHECK(gs1_encoder_setYundercut(ctx, 1));

	// Maxima
	gs1_encoder_setPixMult(ctx, GS1_ENCODERS_MAX_PIXMULT);
	TEST_CHECK(gs1_encoder_setXundercut(ctx, GS1_ENCODERS_MAX_PIXMULT - 1));
	TEST_CHECK(gs1_encoder_setYundercut(ctx, GS1_ENCODERS_MAX_PIXMULT - 1));

	// Must be less than X dimension
	gs1_encoder_setPixMult(ctx, 2);
	TEST_CHECK(!gs1_encoder_setXundercut(ctx, 2));
	TEST_CHECK(!gs1_encoder_setYundercut(ctx, 2));

	// Not negative
	TEST_CHECK(!gs1_encoder_setXundercut(ctx, -1));
	TEST_CHECK(!gs1_encoder_setYundercut(ctx, -1));

	gs1_encoder_free(ctx);

}


static void test_segHt() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	gs1_encoder_setPixMult(ctx, 3);
	TEST_CHECK(gs1_encoder_setSepHt(ctx, 5));
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 5);

	// Range with smallest X dimension
	gs1_encoder_setPixMult(ctx, 1);
	TEST_CHECK(gs1_encoder_setSepHt(ctx, 1));
	TEST_CHECK(!gs1_encoder_setSepHt(ctx, 0));
	TEST_CHECK(gs1_encoder_setSepHt(ctx, 2));
	TEST_CHECK(!gs1_encoder_setSepHt(ctx, 3));

	// Range with largest X dimension
	gs1_encoder_setPixMult(ctx, GS1_ENCODERS_MAX_PIXMULT);
	TEST_CHECK(gs1_encoder_setSepHt(ctx, GS1_ENCODERS_MAX_PIXMULT));
	TEST_CHECK(!gs1_encoder_setSepHt(ctx, GS1_ENCODERS_MAX_PIXMULT - 1));
	TEST_CHECK(gs1_encoder_setSepHt(ctx, 2 * GS1_ENCODERS_MAX_PIXMULT));
	TEST_CHECK(!gs1_encoder_setSepHt(ctx, 2 * GS1_ENCODERS_MAX_PIXMULT + 1));

	gs1_encoder_free(ctx);

}


static void test_segWidth() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setSegWidth(ctx, 6));
	TEST_CHECK(gs1_encoder_getSegWidth(ctx) == 6);
	TEST_CHECK(!gs1_encoder_setSegWidth(ctx, 0));
	TEST_CHECK(!gs1_encoder_setSegWidth(ctx, 1));
	TEST_CHECK(gs1_encoder_setSegWidth(ctx, 2));
	TEST_CHECK(!gs1_encoder_setSegWidth(ctx, 5));    // not even
	TEST_CHECK(gs1_encoder_setSegWidth(ctx, 22));
	TEST_CHECK(!gs1_encoder_setSegWidth(ctx, 23));
	TEST_CHECK(!gs1_encoder_setSegWidth(ctx, 24));

	gs1_encoder_free(ctx);

}


static void test_linHeight() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setLinHeight(ctx, 12));
	TEST_CHECK(gs1_encoder_getLinHeight(ctx) == 12);
	TEST_CHECK(!gs1_encoder_setLinHeight(ctx, 0));
	TEST_CHECK(!gs1_encoder_setLinHeight(ctx, GS1_ENCODERS_MAX_LINHT+1));

	gs1_encoder_free(ctx);

}


static void test_outFile() {

	gs1_encoder* ctx;

	char longfname[GS1_ENCODERS_MAX_FNAME+1];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "a"));

	for (i = 0; i < GS1_ENCODERS_MAX_FNAME; i++) {
		longfname[i]='a';
	}
	TEST_CHECK(!gs1_encoder_setOutFile(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setOutFile(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


static void test_bmp() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setBmp(ctx, false));      // tif
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setBmp(ctx, true));       // now bmp, reset filename
	TEST_CHECK(gs1_encoder_getBmp(ctx) == true);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_BMP_FILE) == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setBmp(ctx, true));       // still bmp, no change
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);

	TEST_CHECK(gs1_encoder_setBmp(ctx, true));       // bmp
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setBmp(ctx, false));      // now tif, reset filename
	TEST_CHECK(gs1_encoder_getBmp(ctx) == false);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setBmp(ctx, false));      // still tiff, change
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);

	TEST_CHECK(gs1_encoder_setBmp(ctx, true));       // bmp
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setBmp(ctx, false));      // now tif, don't reset empty filename
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "") == 0);

	TEST_CHECK(gs1_encoder_setBmp(ctx, false));      // tif
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setBmp(ctx, true));       // now bmp, don't reset empty filename
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "") == 0);

	gs1_encoder_free(ctx);

}


static void test_dataFile() {

	gs1_encoder* ctx;

	char longfname[GS1_ENCODERS_MAX_FNAME+1];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setDataFile(ctx, "test.file"));
	TEST_CHECK(strcmp(gs1_encoder_getDataFile(ctx), "test.file") == 0);
	TEST_CHECK(!gs1_encoder_setDataFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setDataFile(ctx, "a"));

	for (i = 0; i < GS1_ENCODERS_MAX_FNAME; i++) {
		longfname[i]='a';
	}
	TEST_CHECK(!gs1_encoder_setDataFile(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setDataFile(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


static void test_dataStr() {

	gs1_encoder* ctx;

	char longfname[GS1_ENCODERS_MAX_KEYDATA+1];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setDataStr(ctx, "barcode"));
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "barcode") == 0);
	TEST_CHECK(gs1_encoder_setDataStr(ctx, ""));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "a"));

	for (i = 0; i < GS1_ENCODERS_MAX_KEYDATA; i++) {
		longfname[i]='a';
	}
	TEST_CHECK(!gs1_encoder_setDataStr(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setDataStr(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


static void test_getBuffer() {

	gs1_encoder* ctx;
	uint8_t* buf;
	size_t size;
	uint8_t test_tif[] = { 0x49, 0x49, 0x2A, 0x00 };
	uint8_t test_bmp[] = { 0x42, 0x4D, 0xDE, 0x04 };

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_getBuffer(ctx, (void*)&buf) == 0);
	TEST_CHECK(buf == NULL);

	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN13));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "123456789012"));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));

	TEST_CHECK(gs1_encoder_setBmp(ctx, false));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK((size = gs1_encoder_getBuffer(ctx, (void*)&buf)) == 1234);  // Really!
	TEST_CHECK(memcmp(buf, test_tif, sizeof(test_tif)) == 0);

	TEST_CHECK(gs1_encoder_setBmp(ctx, true));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK((size = gs1_encoder_getBuffer(ctx, (void*)&buf)) == 1246);
	TEST_CHECK(memcmp(buf, test_bmp, sizeof(test_bmp)) == 0);

	gs1_encoder_free(ctx);

}


TEST_LIST = {
    { "getVersion", test_getVersion },
    { "defaults", test_defaults },
    { "sym", test_sym },
    { "fileInputFlag", test_fileInputFlag },
    { "pixMult", test_pixMult },
    { "XYundercut", test_XYundercut },
    { "segHt", test_segHt },
    { "segWidth", test_segWidth },
    { "linHeight", test_linHeight },
    { "outFile", test_outFile },
    { "dataFile", test_dataFile },
    { "dataStr", test_dataStr },
    { "bmp", test_bmp },
    { "getBuffer", test_getBuffer },
    { NULL, NULL }
};
