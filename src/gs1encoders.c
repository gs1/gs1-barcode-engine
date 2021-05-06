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

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "enc-private.h"


gs1_encoder* gs1_encoder_init(void) {

	gs1_encoder *ctx = malloc(sizeof(gs1_encoder));
	if (ctx == NULL) return NULL;

	// Set default parameters
	ctx->pixMult = 1;
	ctx->Xundercut = 0;
	ctx->Yundercut = 0;
	ctx->sepHt = 1;
	ctx->bmp = false;
	ctx->segWidth = 22;
	ctx->linHeight = 25;
	strcpy(ctx->outFile, "out.tif");
	ctx->sym = sNONE;
	ctx->inputFlag = 0; // for kbd input
	ctx->errFlag = false;
	ctx->errMsg[0] = '\0';

	return ctx;

}


void gs1_encoder_free(gs1_encoder *ctx) {
	if (ctx == NULL) return;
	free(ctx);
	ctx = NULL;
}


int gs1_encoder_getSym(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->sym;
}
void gs1_encoder_setSym(gs1_encoder *ctx, int sym) {
	if (ctx == NULL) return;
	ctx->sym = sym;
}


int gs1_encoder_getInputFlag(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->inputFlag;
}
void gs1_encoder_setInputFlag(gs1_encoder *ctx, int inputFlag) {
	if (ctx == NULL) return;
	ctx->inputFlag = inputFlag;
}


int gs1_encoder_getPixMult(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->pixMult;
}
void gs1_encoder_setPixMult(gs1_encoder *ctx, int pixMult) {
	if (ctx == NULL) return;
	ctx->pixMult = pixMult;
}


int gs1_encoder_getXundercut(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->Xundercut;
}
void gs1_encoder_setXundercut(gs1_encoder *ctx, int Xundercut) {
	if (ctx == NULL) return;
	ctx->Xundercut = Xundercut;
}


int gs1_encoder_getYundercut(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->Yundercut;
}
void gs1_encoder_setYundercut(gs1_encoder *ctx, int Yundercut) {
	if (ctx == NULL) return;
	ctx->Yundercut = Yundercut;
}

int gs1_encoder_getSepHt(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->sepHt;
}
void gs1_encoder_setSepHt(gs1_encoder *ctx, int sepHt) {
	if (ctx == NULL) return;
	ctx->sepHt = sepHt;
}


int gs1_encoder_getSegWidth(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->segWidth;
}
void gs1_encoder_setSegWidth(gs1_encoder *ctx, int segWidth) {
	if (ctx == NULL) return;
	ctx->segWidth = segWidth;
}


int gs1_encoder_getBmp(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->bmp;
}
void gs1_encoder_setBmp(gs1_encoder *ctx, int bmp) {
	if (ctx == NULL) return;
	ctx->bmp = bmp;
}


int gs1_encoder_getLinHeight(gs1_encoder *ctx) {
	if (ctx == NULL) return -1;
	return ctx->linHeight;
}
void gs1_encoder_setLinHeight(gs1_encoder *ctx, int linHeight) {
	if (ctx == NULL) return;
	ctx->linHeight = linHeight;
}


char* gs1_encoder_getOutFile(gs1_encoder *ctx) {
	if (ctx == NULL) return NULL;
	return ctx->outFile;
}
void gs1_encoder_setOutFile(gs1_encoder *ctx, char* outFile) {
	if (ctx == NULL) return;
	strcpy(ctx->outFile, outFile);
}


char* gs1_encoder_getDataStr(gs1_encoder *ctx) {
	if (ctx == NULL) return NULL;
	return ctx->dataStr;
}
void gs1_encoder_setDataStr(gs1_encoder *ctx, char* dataStr) {
	if (ctx == NULL) return;
	strcpy(ctx->dataStr, dataStr);
}


char* gs1_encoder_getDataFile(gs1_encoder *ctx) {
	if (ctx == NULL) return NULL;
	return ctx->dataFile;
}
void gs1_encoder_setDataFile(gs1_encoder *ctx, char* dataFile) {
	if (ctx == NULL) return;
	strcpy(ctx->dataFile, dataFile);
}


char* gs1_encoder_getErrMsg(gs1_encoder *ctx) {
	if (ctx == NULL) return NULL;
	return ctx->errMsg;
}


bool gs1_encoder_encode(gs1_encoder *ctx) {

	FILE *iFile, *oFile;

	if (!ctx) return false;

	ctx->errMsg[0] = '\0';
	ctx->errFlag = false;

	if (ctx->inputFlag == 1) {
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

	if ((oFile = fopen(ctx->outFile, "wb")) == NULL) {
		sprintf(ctx->errMsg, "UNABLE TO OPEN %s FILE", ctx->outFile);
		ctx->errFlag = true;
		return false;
	}
	ctx->outfp = oFile;

	switch (ctx->sym) {

		case sRSS14:
		case sRSS14T:
			gs1_RSS14(ctx);
			break;

		case sRSS14S:
			gs1_RSS14S(ctx);
			break;

		case sRSS14SO:
			gs1_RSS14SO(ctx);
			break;

		case sRSSLIM:
			gs1_RSSLim(ctx);
			break;

		case sRSSEXP:
			gs1_RSSExp(ctx);
			break;

		case sUPCA:
		case sEAN13:
			gs1_EAN13(ctx);
			break;

		case sUPCE:
			gs1_UPCE(ctx);
			break;

		case sEAN8:
			gs1_EAN8(ctx);
			break;

		case sUCC128_CCA:
			gs1_U128A(ctx);
			break;

		case sUCC128_CCC:
			gs1_U128C(ctx);
			break;

		default:
			sprintf(ctx->errMsg, "Unknown symbology type %d", ctx->sym);
			ctx->errFlag = true;
			break;

	}

	fclose(oFile);

	return !ctx->errFlag;

}
