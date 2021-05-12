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
	strcpy(ctx->outFile, DEFAULT_TIF_FILE);
	ctx->fileInputFlag = false; // for kbd input
	strcpy(ctx->dataStr, "");
	strcpy(ctx->dataFile, "data.txt");

	return ctx;

}


GS1_ENCODERS_API void gs1_encoder_free(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
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
		sprintf(ctx->errMsg, "Range 1 to %d", GS1_ENCODERS_MAX_PIXMULT);
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
		strcpy(ctx->errMsg, "No undercut when 1 pixel per X");
		ctx->errFlag = true;
		return false;
	}
	if (Xundercut < 0 || Xundercut > ctx->pixMult - 1) {
		sprintf(ctx->errMsg, "Range 1 to %d", ctx->pixMult - 1);
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
		strcpy(ctx->errMsg, "No undercut when 1 pixel per X");
		ctx->errFlag = true;
		return false;
	}
	if (Yundercut < 0 || Yundercut > ctx->pixMult - 1) {
		sprintf(ctx->errMsg, "Range 1 to %d", ctx->pixMult - 1);
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
		sprintf(ctx->errMsg, "Range %d to %d", ctx->pixMult, 2 * ctx->pixMult);
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
		strcpy(ctx->errMsg, "Range 2 to 22");
		ctx->errFlag = true;
		return false;
	}
	if (segWidth & 1) {
		strcpy(ctx->errMsg, "Must be even number");
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
		sprintf(ctx->errMsg, "Range 1 to %d", GS1_ENCODERS_MAX_LINHT);
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
	if (strlen(outFile) < 1 || strlen(outFile) > GS1_ENCODERS_MAX_FNAME) {
		sprintf(ctx->errMsg, "Must be 1 to %d characters", GS1_ENCODERS_MAX_FNAME);
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
		sprintf(ctx->errMsg, "Maximum %d characters", GS1_ENCODERS_MAX_KEYDATA);
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
		sprintf(ctx->errMsg, "Must be 1 to %d characters", GS1_ENCODERS_MAX_FNAME);
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
			sprintf(ctx->errMsg, "UNABLE TO OPEN %s FILE", ctx->dataFile);
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
