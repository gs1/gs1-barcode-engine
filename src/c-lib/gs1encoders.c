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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "enc-private.h"
#include "gs1encoders.h"
#include "driver.h"
#include "ean.h"
#include "rss14.h"
#include "rssexp.h"
#include "rsslim.h"
#include "ucc128.h"


static void reset_error(gs1_encoder *ctx) {
	assert(ctx);
	ctx->errFlag = false;
	ctx->errMsg[0] = '\0';
}

GS1_ENCODERS_API gs1_encoder* gs1_encoder_init(void) {

	gs1_encoder *ctx = malloc(sizeof(gs1_encoder));
	if (ctx == NULL) return NULL;

	reset_error(ctx);

	strcpy(ctx->VERSION, __DATE__);

	// Set default parameters
	ctx->sym = gs1_encoder_sNONE;
	ctx->pixMult = 1;
	ctx->Xundercut = 0;
	ctx->Yundercut = 0;
	ctx->sepHt = 1;
	ctx->segWidth = 22;
	ctx->linHeight = 25;
	ctx->bmp = false;
	strcpy(ctx->dataStr, "");
	strcpy(ctx->dataFile, "data.txt");
	ctx->fileInputFlag = false; // for kbd input
	strcpy(ctx->outFile, DEFAULT_TIF_FILE);
	ctx->buffer = NULL;
	ctx->bufferSize = 0;
	ctx->bufferCap = 0;
	return ctx;

}


GS1_ENCODERS_API void gs1_encoder_free(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	free(ctx->buffer);
	free(ctx);
}


GS1_ENCODERS_API char* gs1_encoder_getVersion(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->VERSION;
}


GS1_ENCODERS_API int gs1_encoder_getSym(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->sym;
}
GS1_ENCODERS_API bool gs1_encoder_setSym(gs1_encoder *ctx, int sym) {
	assert(ctx);
	reset_error(ctx);
	if (sym < gs1_encoder_sNONE || sym >= gs1_encoder_sNUMSYMS) {
		strcpy(ctx->errMsg, "Unknown symbology");
		ctx->errFlag = true;
		return false;
	}
	ctx->sym = sym;
	return true;
}


GS1_ENCODERS_API bool gs1_encoder_getFileInputFlag(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->fileInputFlag;
}
GS1_ENCODERS_API bool gs1_encoder_setFileInputFlag(gs1_encoder *ctx, bool fileInputFlag) {
	assert(ctx);
	reset_error(ctx);
	ctx->fileInputFlag = fileInputFlag;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getPixMult(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->pixMult;
}
GS1_ENCODERS_API bool gs1_encoder_setPixMult(gs1_encoder *ctx, int pixMult) {
	assert(ctx);
	reset_error(ctx);
	if (pixMult < 1 || pixMult > GS1_ENCODERS_MAX_PIXMULT) {
		sprintf(ctx->errMsg, "Valid X-dimension range is 1 to %d", GS1_ENCODERS_MAX_PIXMULT);
		ctx->errFlag = true;
		return false;
	}
	ctx->pixMult = pixMult;
	if (pixMult <= ctx->Xundercut)
		ctx->Xundercut = 0;
	if (pixMult <= ctx->Yundercut)
		ctx->Yundercut = 0;
	if (pixMult * 2 < ctx->sepHt || pixMult > ctx->sepHt)
		ctx->sepHt = pixMult;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getXundercut(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->Xundercut;
}
GS1_ENCODERS_API bool gs1_encoder_setXundercut(gs1_encoder *ctx, int Xundercut) {
	assert(ctx);
	reset_error(ctx);
	if (Xundercut != 0 && ctx->pixMult == 1) {
		strcpy(ctx->errMsg, "No X undercut available when 1 pixel per X");
		ctx->errFlag = true;
		return false;
	}
	if (Xundercut < 0 || Xundercut > ctx->pixMult - 1) {
		sprintf(ctx->errMsg, "Valid X undercut range is 1 to %d", ctx->pixMult - 1);
		ctx->errFlag = true;
		return false;
	}
	ctx->Xundercut = Xundercut;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getYundercut(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->Yundercut;
}
GS1_ENCODERS_API bool gs1_encoder_setYundercut(gs1_encoder *ctx, int Yundercut) {
	assert(ctx);
	reset_error(ctx);
	if (Yundercut !=0 && ctx->pixMult == 1) {
		strcpy(ctx->errMsg, "No Y undercut available when 1 pixel per X");
		ctx->errFlag = true;
		return false;
	}
	if (Yundercut < 0 || Yundercut > ctx->pixMult - 1) {
		sprintf(ctx->errMsg, "Valid Y undercut range is 1 to %d", ctx->pixMult - 1);
		ctx->errFlag = true;
		return false;
	}
	ctx->Yundercut = Yundercut;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getSepHt(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->sepHt;
}
GS1_ENCODERS_API bool gs1_encoder_setSepHt(gs1_encoder *ctx, int sepHt) {
	assert(ctx);
	reset_error(ctx);
	if (sepHt < ctx->pixMult || sepHt > 2 * ctx->pixMult) {
		sprintf(ctx->errMsg, "Valid separator height range is %d to %d", ctx->pixMult, 2 * ctx->pixMult);
		ctx->errFlag = true;
		return false;
	}
	ctx->sepHt = sepHt;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getSegWidth(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->segWidth;
}
GS1_ENCODERS_API bool gs1_encoder_setSegWidth(gs1_encoder *ctx, int segWidth) {
	assert(ctx);
	reset_error(ctx);
	if (segWidth < 2 || segWidth > 22) {
		strcpy(ctx->errMsg, "Valid number of segments range is 2 to 22");
		ctx->errFlag = true;
		return false;
	}
	if (segWidth & 1) {
		strcpy(ctx->errMsg, "Number of segments must be even");
		ctx->errFlag = true;
		return false;
	}
	ctx->segWidth = segWidth;
	return true;
}


GS1_ENCODERS_API bool gs1_encoder_getBmp(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->bmp;
}
GS1_ENCODERS_API bool gs1_encoder_setBmp(gs1_encoder *ctx, bool bmp) {
	assert(ctx);
	reset_error(ctx);
	if (ctx->bmp == bmp) return true;
	if (strcmp(ctx->outFile, "") != 0)
		strcpy(ctx->outFile, bmp ? DEFAULT_BMP_FILE : DEFAULT_TIF_FILE);
	ctx->bmp = bmp;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getLinHeight(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->linHeight;
}
GS1_ENCODERS_API bool gs1_encoder_setLinHeight(gs1_encoder *ctx, int linHeight) {
	assert(ctx);
	reset_error(ctx);
	if (linHeight < 1 || linHeight > GS1_ENCODERS_MAX_LINHT) {
		sprintf(ctx->errMsg, "Valid linear component height range is 1 to %d", GS1_ENCODERS_MAX_LINHT);
		ctx->errFlag = true;
		return false;
	}
	ctx->linHeight = linHeight;
	return true;
}


GS1_ENCODERS_API char* gs1_encoder_getOutFile(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->outFile;
}
GS1_ENCODERS_API bool gs1_encoder_setOutFile(gs1_encoder *ctx, char* outFile) {
	assert(ctx);
	reset_error(ctx);
	if (strlen(outFile) > GS1_ENCODERS_MAX_FNAME) {
		sprintf(ctx->errMsg, "Maximum output file is %d characters", GS1_ENCODERS_MAX_FNAME);
		ctx->errFlag = true;
		return false;
	}
	strcpy(ctx->outFile, outFile);
	return true;
}


GS1_ENCODERS_API char* gs1_encoder_getDataStr(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->dataStr;
}
GS1_ENCODERS_API bool gs1_encoder_setDataStr(gs1_encoder *ctx, char* dataStr) {
	assert(ctx);
	reset_error(ctx);
	if (strlen(dataStr) > GS1_ENCODERS_MAX_KEYDATA) {
		sprintf(ctx->errMsg, "Maximum data length is %d characters", GS1_ENCODERS_MAX_KEYDATA);
		ctx->errFlag = true;
		return false;
	}
	strcpy(ctx->dataStr, dataStr);
	return true;
}


GS1_ENCODERS_API char* gs1_encoder_getDataFile(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->dataFile;
}
GS1_ENCODERS_API bool gs1_encoder_setDataFile(gs1_encoder *ctx, char* dataFile) {
	assert(ctx);
	reset_error(ctx);
	if (strlen(dataFile) < 1 || strlen(dataFile) > GS1_ENCODERS_MAX_FNAME) {
		sprintf(ctx->errMsg, "Input file must be 1 to %d characters", GS1_ENCODERS_MAX_FNAME);
		ctx->errFlag = true;
		return false;
	}
	strcpy(ctx->dataFile, dataFile);
	return true;
}


GS1_ENCODERS_API char* gs1_encoder_getErrMsg(gs1_encoder *ctx) {
	assert(ctx);
	return ctx->errMsg;
}


GS1_ENCODERS_API bool gs1_encoder_encode(gs1_encoder *ctx) {

	FILE *iFile;

	assert(ctx);
	reset_error(ctx);

	if (ctx->fileInputFlag) {
		size_t i;
		if ((iFile = fopen(ctx->dataFile, "r")) == NULL) {
			sprintf(ctx->errMsg, "Unable to open input file: %s", ctx->dataFile);
			ctx->errFlag = true;
			return false;
		}
		i = fread(ctx->dataStr, sizeof(char), GS1_ENCODERS_MAX_DATA, iFile);
		while (i > 0 && ctx->dataStr[i-1] < 32) i--; // strip trailing CRLF etc.
		ctx->dataStr[i] = '\0';
		fclose(iFile);
	}

	switch (ctx->sym) {

		case gs1_encoder_sRSS14:
		case gs1_encoder_sRSS14T:
			gs1_RSS14(ctx);
			break;

		case gs1_encoder_sRSS14S:
			gs1_RSS14S(ctx);
			break;

		case gs1_encoder_sRSS14SO:
			gs1_RSS14SO(ctx);
			break;

		case gs1_encoder_sRSSLIM:
			gs1_RSSLim(ctx);
			break;

		case gs1_encoder_sRSSEXP:
			gs1_RSSExp(ctx);
			break;

		case gs1_encoder_sUPCA:
		case gs1_encoder_sEAN13:
			gs1_EAN13(ctx);
			break;

		case gs1_encoder_sUPCE:
			gs1_UPCE(ctx);
			break;

		case gs1_encoder_sEAN8:
			gs1_EAN8(ctx);
			break;

		case gs1_encoder_sUCC128_CCA:
			gs1_U128A(ctx);
			break;

		case gs1_encoder_sUCC128_CCC:
			gs1_U128C(ctx);
			break;

		default:
			sprintf(ctx->errMsg, "Unknown symbology type %d", ctx->sym);
			ctx->errFlag = true;
			break;

	}

	return !ctx->errFlag;

}


GS1_ENCODERS_API size_t gs1_encoder_getBuffer(gs1_encoder *ctx, void** out) {
	assert(ctx);
	if (!ctx->buffer) {
		*out = NULL;
		return 0;
	}
	*out = ctx->buffer;
	return ctx->bufferSize;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"


void test_api_getVersion() {

	gs1_encoder* ctx;
	char* version;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	version = gs1_encoder_getVersion(ctx);
	TEST_CHECK(version != NULL && strlen(version) > 0);

	gs1_encoder_free(ctx);

}


void test_api_defaults() {

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


void test_api_sym() {

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


void test_api_fileInputFlag() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setFileInputFlag(ctx, true));     // dataFile
	TEST_CHECK(gs1_encoder_getFileInputFlag(ctx) == true);

	gs1_encoder_free(ctx);

}


void test_api_pixMult() {

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


void test_api_XYundercut() {

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


void test_api_sepHt() {

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


void test_api_segWidth() {

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


void test_api_linHeight() {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setLinHeight(ctx, 12));
	TEST_CHECK(gs1_encoder_getLinHeight(ctx) == 12);
	TEST_CHECK(!gs1_encoder_setLinHeight(ctx, 0));
	TEST_CHECK(!gs1_encoder_setLinHeight(ctx, GS1_ENCODERS_MAX_LINHT+1));

	gs1_encoder_free(ctx);

}


void test_api_outFile() {

	gs1_encoder* ctx;

	char longfname[GS1_ENCODERS_MAX_FNAME+2];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "a"));

	for (i = 0; i < GS1_ENCODERS_MAX_FNAME; i++) {
		longfname[i]='a';
	}
	longfname[i+1]='\0';
	TEST_CHECK(!gs1_encoder_setOutFile(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setOutFile(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


void test_api_bmp() {

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


void test_api_dataFile() {

	gs1_encoder* ctx;

	char longfname[GS1_ENCODERS_MAX_FNAME+2];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setDataFile(ctx, "test.file"));
	TEST_CHECK(strcmp(gs1_encoder_getDataFile(ctx), "test.file") == 0);
	TEST_CHECK(!gs1_encoder_setDataFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setDataFile(ctx, "a"));

	for (i = 0; i < GS1_ENCODERS_MAX_FNAME; i++) {
		longfname[i]='a';
	}
	longfname[i+1]='\0';
	TEST_CHECK(!gs1_encoder_setDataFile(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setDataFile(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


void test_api_dataStr() {

	gs1_encoder* ctx;

	char longfname[GS1_ENCODERS_MAX_KEYDATA+2];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init()) != NULL);

	TEST_CHECK(gs1_encoder_setDataStr(ctx, "barcode"));
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "barcode") == 0);
	TEST_CHECK(gs1_encoder_setDataStr(ctx, ""));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "a"));

	for (i = 0; i < GS1_ENCODERS_MAX_KEYDATA; i++) {
		longfname[i]='a';
	}
	longfname[i+1]='\0';
	TEST_CHECK(!gs1_encoder_setDataStr(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setDataStr(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


void test_api_getBuffer() {

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
	TEST_ASSERT(buf != NULL);
	TEST_CHECK(memcmp(buf, test_tif, sizeof(test_tif)) == 0);

	TEST_CHECK(gs1_encoder_setBmp(ctx, true));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK((size = gs1_encoder_getBuffer(ctx, (void*)&buf)) == 1246);
	TEST_ASSERT(buf != NULL);
	TEST_CHECK(memcmp(buf, test_bmp, sizeof(test_bmp)) == 0);

	// Check integrity of dataStr after encode
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN13));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "123456789012|99123456"));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setBmp(ctx, true));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx),"123456789012|99123456") == 0);

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
