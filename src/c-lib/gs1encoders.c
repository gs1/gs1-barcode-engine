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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enc-private.h"
#include "gs1encoders.h"
#include "driver.h"
#include "dm.h"
#include "ean.h"
#include "gs1.h"
#include "rss14.h"
#include "rssexp.h"
#include "rsslim.h"
#include "scandata.h"
#include "ucc128.h"
#include "qr.h"


static void reset_error(gs1_encoder *ctx) {
	assert(ctx);
	ctx->errFlag = false;
	ctx->errMsg[0] = '\0';
}


static void free_bufferStrings(gs1_encoder *ctx) {
	int i = 0;
	assert(ctx);
	if (ctx->bufferStrings) {
		while (ctx->bufferStrings[i])
			free(ctx->bufferStrings[i++]);
		free(ctx->bufferStrings);
		ctx->bufferStrings = NULL;
	}
}


GS1_ENCODERS_API size_t gs1_encoder_instanceSize(void) {
	return sizeof(struct gs1_encoder);
}


GS1_ENCODERS_API int gs1_encoder_getMaxGS1_128LinearHeight(void) {
	return UCC128_MAX_LINHT;
}


GS1_ENCODERS_API int gs1_encoder_getMaxFilenameLength(void) {
	return MAX_FNAME;
}


GS1_ENCODERS_API int gs1_encoder_getMaxDataStrLength(void) {
	return MAX_DATA;

}


GS1_ENCODERS_API int gs1_encoder_getMaxPixMult(void) {
	return MAX_PIXMULT;
}


GS1_ENCODERS_API gs1_encoder* gs1_encoder_init(void *mem) {

	gs1_encoder *ctx = NULL;

	if (!mem) {  // No storage provided so allocate our own
#ifndef NOMALLOC
		ctx = malloc(sizeof(gs1_encoder));
#endif
		if (ctx == NULL) return NULL;
		ctx->localAlloc = true;
	} else {  // Use the provided storage
		ctx = mem;
		ctx->localAlloc = false;
	}

	reset_error(ctx);

	// Set default parameters
	ctx->sym = gs1_encoder_sNONE;
	ctx->pixMult = 1;
	ctx->Xundercut = 0;
	ctx->Yundercut = 0;
	ctx->sepHt = 1;
	ctx->dataBarExpandedSegmentsWidth = 22;
	ctx->gs1_128LinearHeight = 25;
	ctx->dmRows = 0;
	ctx->dmCols = 0;
	ctx->qrEClevel = gs1_encoder_qrEClevelM;
	ctx->qrVersion = 0;  // Automatic
	ctx->addCheckDigit = false;
	ctx->format = gs1_encoder_dTIF;
	strcpy(ctx->dataStr, "");
	ctx->numAIs = 0;
	strcpy(ctx->dataFile, "data.txt");
	ctx->fileInputFlag = false; // for kbd input
	strcpy(ctx->outFile, DEFAULT_TIF_FILE);
	ctx->buffer = NULL;
	ctx->bufferSize = 0;
	ctx->bufferCap = 0;
	ctx->bufferWidth = 0;
	ctx->bufferHeight = 0;
	ctx->bufferStrings = NULL;
	return ctx;

}


GS1_ENCODERS_API void gs1_encoder_free(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	free_bufferStrings(ctx);
	free(ctx->buffer);
	if (ctx->localAlloc)
		free(ctx);
}


GS1_ENCODERS_API char* gs1_encoder_getVersion(void) {
	return __DATE__;
}


GS1_ENCODERS_API int gs1_encoder_getSym(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->sym;
}
GS1_ENCODERS_API bool gs1_encoder_setSym(gs1_encoder *ctx, const int sym) {
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
GS1_ENCODERS_API bool gs1_encoder_setFileInputFlag(gs1_encoder *ctx, const bool fileInputFlag) {
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
GS1_ENCODERS_API bool gs1_encoder_setPixMult(gs1_encoder *ctx, const int pixMult) {
	assert(ctx);
	reset_error(ctx);
	if (pixMult < 1 || pixMult > MAX_PIXMULT) {
		sprintf(ctx->errMsg, "Valid X-dimension range is 1 to %d", MAX_PIXMULT);
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
GS1_ENCODERS_API bool gs1_encoder_setXundercut(gs1_encoder *ctx, const int Xundercut) {
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
GS1_ENCODERS_API bool gs1_encoder_setYundercut(gs1_encoder *ctx, const int Yundercut) {
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
GS1_ENCODERS_API bool gs1_encoder_setSepHt(gs1_encoder *ctx, const int sepHt) {
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


GS1_ENCODERS_API int gs1_encoder_getDataBarExpandedSegmentsWidth(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->dataBarExpandedSegmentsWidth;
}
GS1_ENCODERS_API bool gs1_encoder_setDataBarExpandedSegmentsWidth(gs1_encoder *ctx, const int dataBarExpandedSegmentsWidth) {
	assert(ctx);
	reset_error(ctx);
	if (dataBarExpandedSegmentsWidth < 2 || dataBarExpandedSegmentsWidth > 22) {
		strcpy(ctx->errMsg, "Valid number of segments range is 2 to 22");
		ctx->errFlag = true;
		return false;
	}
	if (dataBarExpandedSegmentsWidth & 1) {
		strcpy(ctx->errMsg, "Number of segments must be even");
		ctx->errFlag = true;
		return false;
	}
	ctx->dataBarExpandedSegmentsWidth = dataBarExpandedSegmentsWidth;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getDmRows(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->dmRows;
}
GS1_ENCODERS_API bool gs1_encoder_setDmRows(gs1_encoder *ctx, const int rows) {
	assert(ctx);
	reset_error(ctx);
	switch (rows) {
		case gs1_encoder_dmRowsAutomatic:
		case gs1_encoder_dmRows8:
		case gs1_encoder_dmRows10:
		case gs1_encoder_dmRows12:
		case gs1_encoder_dmRows14:
		case gs1_encoder_dmRows16:
		case gs1_encoder_dmRows18:
		case gs1_encoder_dmRows20:
		case gs1_encoder_dmRows22:
		case gs1_encoder_dmRows24:
		case gs1_encoder_dmRows26:
		case gs1_encoder_dmRows32:
		case gs1_encoder_dmRows36:
		case gs1_encoder_dmRows40:
		case gs1_encoder_dmRows44:
		case gs1_encoder_dmRows48:
		case gs1_encoder_dmRows52:
		case gs1_encoder_dmRows64:
		case gs1_encoder_dmRows72:
		case gs1_encoder_dmRows80:
		case gs1_encoder_dmRows88:
		case gs1_encoder_dmRows96:
		case gs1_encoder_dmRows104:
		case gs1_encoder_dmRows120:
		case gs1_encoder_dmRows132:
		case gs1_encoder_dmRows144:
			ctx->dmRows = rows;
			break;
		default:
			strcpy(ctx->errMsg, "Valid number of Data Matrix rows range is 8 to 144, or 0");
			ctx->errFlag = true;
			return false;
	}
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getDmColumns(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->dmCols;
}
GS1_ENCODERS_API bool gs1_encoder_setDmColumns(gs1_encoder *ctx, const int columns) {
	assert(ctx);
	reset_error(ctx);
	switch (columns) {
		case gs1_encoder_dmColumnsAutomatic:
		case gs1_encoder_dmColumns10:
		case gs1_encoder_dmColumns12:
		case gs1_encoder_dmColumns14:
		case gs1_encoder_dmColumns16:
		case gs1_encoder_dmColumns18:
		case gs1_encoder_dmColumns20:
		case gs1_encoder_dmColumns22:
		case gs1_encoder_dmColumns24:
		case gs1_encoder_dmColumns26:
		case gs1_encoder_dmColumns32:
		case gs1_encoder_dmColumns36:
		case gs1_encoder_dmColumns40:
		case gs1_encoder_dmColumns44:
		case gs1_encoder_dmColumns48:
		case gs1_encoder_dmColumns52:
		case gs1_encoder_dmColumns64:
		case gs1_encoder_dmColumns72:
		case gs1_encoder_dmColumns80:
		case gs1_encoder_dmColumns88:
		case gs1_encoder_dmColumns96:
		case gs1_encoder_dmColumns104:
		case gs1_encoder_dmColumns120:
		case gs1_encoder_dmColumns132:
		case gs1_encoder_dmColumns144:
			ctx->dmCols = columns;
			break;
		default:
			strcpy(ctx->errMsg, "Valid number of Data Matrix columns range is 10 to 144, or 0");
			ctx->errFlag = true;
			return false;
	}
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getQrVersion(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->qrVersion;
}
GS1_ENCODERS_API bool gs1_encoder_setQrVersion(gs1_encoder *ctx, const int version) {
	assert(ctx);
	reset_error(ctx);
	switch (version) {
		case gs1_encoder_qrVersionAutomatic:
		case gs1_encoder_qrVersion1:
		case gs1_encoder_qrVersion2:
		case gs1_encoder_qrVersion3:
		case gs1_encoder_qrVersion4:
		case gs1_encoder_qrVersion5:
		case gs1_encoder_qrVersion6:
		case gs1_encoder_qrVersion7:
		case gs1_encoder_qrVersion8:
		case gs1_encoder_qrVersion9:
		case gs1_encoder_qrVersion10:
		case gs1_encoder_qrVersion11:
		case gs1_encoder_qrVersion12:
		case gs1_encoder_qrVersion13:
		case gs1_encoder_qrVersion14:
		case gs1_encoder_qrVersion15:
		case gs1_encoder_qrVersion16:
		case gs1_encoder_qrVersion17:
		case gs1_encoder_qrVersion18:
		case gs1_encoder_qrVersion19:
		case gs1_encoder_qrVersion20:
		case gs1_encoder_qrVersion21:
		case gs1_encoder_qrVersion22:
		case gs1_encoder_qrVersion23:
		case gs1_encoder_qrVersion24:
		case gs1_encoder_qrVersion25:
		case gs1_encoder_qrVersion26:
		case gs1_encoder_qrVersion27:
		case gs1_encoder_qrVersion28:
		case gs1_encoder_qrVersion29:
		case gs1_encoder_qrVersion30:
		case gs1_encoder_qrVersion31:
		case gs1_encoder_qrVersion32:
		case gs1_encoder_qrVersion33:
		case gs1_encoder_qrVersion34:
		case gs1_encoder_qrVersion35:
		case gs1_encoder_qrVersion36:
		case gs1_encoder_qrVersion37:
		case gs1_encoder_qrVersion38:
		case gs1_encoder_qrVersion39:
		case gs1_encoder_qrVersion40:
			ctx->qrVersion = version;
			break;
		default:
			strcpy(ctx->errMsg, "Valid QR Code version 1 to 40, or 0");
			ctx->errFlag = true;
			return false;
	}
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getQrEClevel(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->qrEClevel;
}
GS1_ENCODERS_API bool gs1_encoder_setQrEClevel(gs1_encoder *ctx, const int ecLevel) {
	assert(ctx);
	reset_error(ctx);
	switch (ecLevel) {
		case gs1_encoder_qrEClevelL:
		case gs1_encoder_qrEClevelM:
		case gs1_encoder_qrEClevelQ:
		case gs1_encoder_qrEClevelH:
 			ctx->qrEClevel = ecLevel;
			break;
		default:
			sprintf(ctx->errMsg, "Valid QR Code error correction level values are L=%d, M=%d, Q=%d, H=%d",
				gs1_encoder_qrEClevelL,
				gs1_encoder_qrEClevelM,
				gs1_encoder_qrEClevelQ,
				gs1_encoder_qrEClevelH);
			ctx->errFlag = true;
			return false;
	}
	return true;
}


GS1_ENCODERS_API bool gs1_encoder_getAddCheckDigit(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->addCheckDigit;
}
GS1_ENCODERS_API bool gs1_encoder_setAddCheckDigit(gs1_encoder *ctx, const bool addCheckDigit) {
	assert(ctx);
	reset_error(ctx);
	ctx->addCheckDigit = addCheckDigit;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getFormat(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->format;
}
GS1_ENCODERS_API bool gs1_encoder_setFormat(gs1_encoder *ctx, const int format) {
	assert(ctx);
	reset_error(ctx);
	if (ctx->format == format) return true;
	if (strcmp(ctx->outFile, "") != 0) {
		switch (format) {
			case gs1_encoder_dBMP:
				strcpy(ctx->outFile, DEFAULT_BMP_FILE);
				break;
			case gs1_encoder_dTIF:
				strcpy(ctx->outFile, DEFAULT_TIF_FILE);
				break;
			case gs1_encoder_dRAW:
				strcpy(ctx->outFile, "");
				break;
			default:     // No such format
				return false;
		}
	}
	ctx->format = format;
	return true;
}


GS1_ENCODERS_API int gs1_encoder_getGS1_128LinearHeight(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->gs1_128LinearHeight;
}
GS1_ENCODERS_API bool gs1_encoder_setGS1_128LinearHeight(gs1_encoder *ctx, const int gs1_128LinearHeight) {
	assert(ctx);
	reset_error(ctx);
	if (gs1_128LinearHeight < 1 || gs1_128LinearHeight > UCC128_MAX_LINHT) {
		sprintf(ctx->errMsg, "Valid linear component height range is 1 to %d", UCC128_MAX_LINHT);
		ctx->errFlag = true;
		return false;
	}
	ctx->gs1_128LinearHeight = gs1_128LinearHeight;
	return true;
}


GS1_ENCODERS_API char* gs1_encoder_getOutFile(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->outFile;
}
GS1_ENCODERS_API bool gs1_encoder_setOutFile(gs1_encoder *ctx, const char* outFile) {
	assert(ctx);
	assert(outFile);
	reset_error(ctx);
	if (strlen(outFile) > MAX_FNAME) {
		sprintf(ctx->errMsg, "Maximum output file is %d characters", MAX_FNAME);
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
GS1_ENCODERS_API bool gs1_encoder_setDataStr(gs1_encoder *ctx, const char* dataStr) {

	char *cc;

	assert(ctx);
	assert(dataStr);
	reset_error(ctx);

	if (strlen(dataStr) > MAX_DATA) {
		sprintf(ctx->errMsg, "Maximum data length is %d characters", MAX_DATA);
		ctx->errFlag = true;
		return false;
	}
	if (ctx->dataStr != dataStr)				// For file input we are given ctx->dataStr
		strcpy(ctx->dataStr, dataStr);

	// Validate and process data, including extraction of HRI
	ctx->numAIs = 0;
	if ((cc = strchr(ctx->dataStr, '|')) != NULL)		// Composite symbol
	{
		*cc = '\0';					// Delimit end of linear component
		if (*ctx->dataStr == '#' && !gs1_processAIdata(ctx, ctx->dataStr))
			goto fail;
		ctx->aiData[ctx->numAIs++].aiEntry = NULL;	// Indicate separator in HRI
		if (!gs1_processAIdata(ctx, cc + 1))
			goto fail;
		*cc = '|';					// Restore orginal "|"
	}
	else {							// Linear-only symbol
		if (*ctx->dataStr == '#' && !gs1_processAIdata(ctx, ctx->dataStr))
			goto fail;
	}

	return true;

fail:

	*ctx->dataStr = '\0';
	ctx->numAIs = 0;
	return false;

}


GS1_ENCODERS_API bool gs1_encoder_setAIdataStr(gs1_encoder *ctx, const char* gs1data) {

	char *cc;

	assert(ctx);
	assert(gs1data);
	reset_error(ctx);

	// Validate GS1 data
	ctx->numAIs = 0;
	if ((cc = strchr(gs1data, '|')) != NULL)		// Composite symbol
	{
		*cc = '\0';					// Delimit end of linear component
		if (!gs1_parseAIdata(ctx, gs1data, ctx->dataStr)) {
			*ctx->dataStr = '\0';
			ctx->numAIs = 0;
			return false;
		}
		if (ctx->numAIs >= MAX_AIS) {
			strcpy(ctx->errMsg, "Too many AIs");
			ctx->errFlag = true;
			*ctx->dataStr = '\0';
			ctx->numAIs = 0;
			return false;
		}
		strcat(ctx->dataStr, "|");
		ctx->aiData[ctx->numAIs++].aiEntry = NULL;	// Indicate separator in HRI
		if (!gs1_parseAIdata(ctx, cc+1, ctx->dataStr + strlen(ctx->dataStr))) {
			*ctx->dataStr = '\0';
			ctx->numAIs = 0;
			return false;
		}
		*cc = '|';					// Restore orginal "|"
	}
	else {							// Linear-only symbol
		if (!gs1_parseAIdata(ctx, gs1data, ctx->dataStr)) {
			*ctx->dataStr = '\0';
			ctx->numAIs = 0;
			return false;
		}
	}

	return true;

}


GS1_ENCODERS_API char* gs1_encoder_getAIdataStr(gs1_encoder *ctx) {

	int i, j;
	struct aiValue ai;
	char *p = ctx->outStr;

	assert(ctx);
	assert(ctx->numAIs <= MAX_AIS);
	reset_error(ctx);

	if (ctx->numAIs == 0)		// Not GS1 data
		return NULL;

	for (i = 0; i < ctx->numAIs; i++) {
		ai = ctx->aiData[i];
		if (ai.aiEntry) {
			p += sprintf(p, "(%s)", ai.aiEntry->ai);
			for (j = 0; j < ai.vallen; j++) {
				if (ai.value[j] == '(')		// Escape data "("
					*p++ = '\\';
				*p++ = ai.value[j];
			}
		} else {
			*p++ = '|';
		}
	}
	*p = '\0';

	return ctx->outStr;

}


GS1_ENCODERS_API char* gs1_encoder_getScanData(gs1_encoder* ctx) {
	assert(ctx);
	return gs1_generateScanData(ctx);
}


GS1_ENCODERS_API bool gs1_encoder_setScanData(gs1_encoder* ctx, const char *scanData) {
	assert(ctx);
	assert(scanData);
	return gs1_processScanData(ctx);
}


GS1_ENCODERS_API int gs1_encoder_getHRI(gs1_encoder *ctx, char*** out) {

	int i;
	struct aiValue ai;
	char *p = ctx->outStr;

	assert(ctx);
	assert(ctx->numAIs <= MAX_AIS);
	reset_error(ctx);

	*p = '\0';
	for (i = 0; i < ctx->numAIs; i++) {
		ctx->outHRI[i] = p;
		ai = ctx->aiData[i];
		if (ai.aiEntry)
			p += sprintf(p, "(%s) %.*s", ai.aiEntry->ai, ai.vallen, ai.value);
		else
			p += sprintf(p, "--");
		*p++ = '\0';
	}

	*out = ctx->outHRI;
	return ctx->numAIs;

}


GS1_ENCODERS_API char* gs1_encoder_getDataFile(gs1_encoder *ctx) {
	assert(ctx);
	reset_error(ctx);
	return ctx->dataFile;
}
GS1_ENCODERS_API bool gs1_encoder_setDataFile(gs1_encoder *ctx, const char* dataFile) {
	assert(ctx);
	reset_error(ctx);
	if (strlen(dataFile) < 1 || strlen(dataFile) > MAX_FNAME) {
		sprintf(ctx->errMsg, "Input file must be 1 to %d characters", MAX_FNAME);
		ctx->errFlag = true;
		return false;
	}
	strcpy(ctx->dataFile, dataFile);
	return true;
}


GS1_ENCODERS_API char* gs1_encoder_getErrMsg(gs1_encoder *ctx) {
	assert(ctx);
	assert(!ctx->errFlag ^ *ctx->errMsg);
	return ctx->errMsg;
}


GS1_ENCODERS_API bool gs1_encoder_encode(gs1_encoder *ctx) {

	FILE *iFile;

	assert(ctx);
	reset_error(ctx);

	free_bufferStrings(ctx);
	free(ctx->buffer);
	ctx->buffer = NULL;
	ctx->bufferCap = 0;
	ctx->bufferSize = 0;
	ctx->bufferWidth = 0;
	ctx->bufferHeight = 0;

	if (ctx->fileInputFlag) {
		size_t i;
		if ((iFile = fopen(ctx->dataFile, "r")) == NULL) {
			sprintf(ctx->errMsg, "Unable to open input file: %s", ctx->dataFile);
			ctx->errFlag = true;
			return false;
		}
		i = fread(ctx->dataStr, sizeof(char), MAX_DATA, iFile);
		while (i > 0 && ctx->dataStr[i-1] < 32) i--;		// Strip trailing CRLF etc.
		ctx->dataStr[i] = '\0';
		fclose(iFile);
		if (!gs1_encoder_setDataStr(ctx, ctx->dataStr))		// Process the input
			return false;
	}

	switch (ctx->sym) {

		case gs1_encoder_sDataBarOmni:
		case gs1_encoder_sDataBarTruncated:
			gs1_RSS14(ctx);
			break;

		case gs1_encoder_sDataBarStacked:
			gs1_RSS14S(ctx);
			break;

		case gs1_encoder_sDataBarStackedOmni:
			gs1_RSS14SO(ctx);
			break;

		case gs1_encoder_sDataBarLimited:
			gs1_RSSLim(ctx);
			break;

		case gs1_encoder_sDataBarExpanded:
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

		case gs1_encoder_sGS1_128_CCA:
			gs1_U128A(ctx);
			break;

		case gs1_encoder_sGS1_128_CCC:
			gs1_U128C(ctx);
			break;

		case gs1_encoder_sQR:
			gs1_QR(ctx);
			break;

		case gs1_encoder_sDM:
			gs1_DM(ctx);
			break;

		default:
			sprintf(ctx->errMsg, "Unknown symbology type %d", ctx->sym);
			ctx->errFlag = true;
			break;

	}

	if (ctx->errFlag) {
		assert(!ctx->buffer && ctx->bufferCap == 0 && ctx->bufferSize == 0 &&
			ctx->bufferWidth == 0 && ctx->bufferHeight == 0);
		return false;
	}

	return true;

}


GS1_ENCODERS_API size_t gs1_encoder_getBuffer(gs1_encoder *ctx, void** out) {
	assert(ctx);

	if (!ctx->buffer) {
		assert(ctx->bufferSize == 0);
		*out = NULL;
		return 0;
	}

	assert(ctx->bufferSize > 0);

	*out = ctx->buffer;
	return ctx->bufferSize;
}


GS1_ENCODERS_API size_t gs1_encoder_getBufferStrings(gs1_encoder *ctx, char*** out) {

	uint8_t *buf;
	int w, h, bw, x, y;

	assert(ctx);

	if (!ctx->buffer) {
		*out = NULL;
		return 0;
	}

	assert(ctx->bufferHeight > 0);
	assert(ctx->bufferWidth > 0);

	if (ctx->bufferStrings) goto out;

	buf = ctx->buffer;
	w = ctx->bufferWidth;
	h = ctx->bufferHeight;
	bw = (w-1)/8+1;

	ctx->bufferStrings = malloc((size_t)(h+1) * sizeof(char*));
	if (!ctx->bufferStrings)
		return 0;

	for (y = 0; y < h; y++) {
		if (!(ctx->bufferStrings[y] = malloc((size_t)(w+1) * sizeof(char)))) {
			free_bufferStrings(ctx);
			return 0;
		}
		for (x = 0; x < w; x++) {
			ctx->bufferStrings[y][x] = (buf[bw*y + x/8] >> (7-x%8) & 1) ? 'X' : ' ';
		}
		ctx->bufferStrings[y][x] = '\0';
	}
	ctx->bufferStrings[h] = NULL;

out:
	*out = ctx->bufferStrings;
	return (size_t)ctx->bufferHeight;
}


GS1_ENCODERS_API int gs1_encoder_getBufferWidth(gs1_encoder *ctx) {
	assert(ctx);
	assert(!ctx->buffer ^ (ctx->bufferWidth > 0));
	return ctx->bufferWidth;
}


GS1_ENCODERS_API int gs1_encoder_getBufferHeight(gs1_encoder *ctx) {
	assert(ctx);
	assert(!ctx->buffer ^ (ctx->bufferHeight > 0));
	return ctx->bufferHeight;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"


// Sizable buffer on the heap so that we don't exhaust the stack
char bigbuffer[MAX_DATA+2];


void test_api_getVersion(void) {
	char *version = gs1_encoder_getVersion();

	TEST_CHECK(version != NULL && strcmp(version, __DATE__) == 0);
}


void test_api_instanceSize(void) {
	TEST_CHECK(gs1_encoder_instanceSize() == sizeof(struct gs1_encoder));
}


void test_api_maxUcc128LinHeight(void) {
	TEST_CHECK(gs1_encoder_getMaxGS1_128LinearHeight() == UCC128_MAX_LINHT);
}


void test_api_maxFilenameLength(void) {
	TEST_CHECK(gs1_encoder_getMaxFilenameLength() == MAX_FNAME);
}


void test_api_maxInputBuffer(void) {
	TEST_CHECK(gs1_encoder_getMaxDataStrLength() == MAX_DATA);
}


void test_api_maxPixMult(void) {
	TEST_CHECK(gs1_encoder_getMaxPixMult() == MAX_PIXMULT);
}


void test_api_init(void) {

	gs1_encoder* ctx;
	void *heap;
	size_t mem;
	uint8_t stack[sizeof(gs1_encoder)];

	// Mallocs its own memory
	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_getFormat(ctx) == gs1_encoder_dTIF);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);
	gs1_encoder_free(ctx);

	// We malloc the storage on the heap and pass it in
	TEST_ASSERT((mem = gs1_encoder_instanceSize()) > 0);
	TEST_ASSERT((heap = malloc(mem)) != NULL);
	TEST_ASSERT((ctx = gs1_encoder_init(heap)) == heap);
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_getFormat(ctx) == gs1_encoder_dTIF);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);
	gs1_encoder_free(ctx);

	// We malloc the storage on the stack and pass it in
	TEST_ASSERT((ctx = gs1_encoder_init(&stack)) != NULL);
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_getFormat(ctx) == gs1_encoder_dTIF);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);

}


void test_api_defaults(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_getPixMult(ctx) == 1);
	TEST_CHECK(gs1_encoder_getXundercut(ctx) == 0);
	TEST_CHECK(gs1_encoder_getYundercut(ctx) == 0);
	TEST_CHECK(gs1_encoder_getSepHt(ctx) == 1);
	TEST_CHECK(gs1_encoder_getDataBarExpandedSegmentsWidth(ctx) == 22);
	TEST_CHECK(gs1_encoder_getGS1_128LinearHeight(ctx) == 25);
	TEST_CHECK(gs1_encoder_getFormat(ctx) == gs1_encoder_dTIF);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);
	TEST_CHECK(gs1_encoder_getFileInputFlag(ctx) == false);    // dataStr
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "") == 0);
	TEST_CHECK(strcmp(gs1_encoder_getDataFile(ctx), "data.txt") == 0);

	gs1_encoder_free(ctx);

}


void test_api_sym(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarOmni));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarTruncated));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarStacked));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarStackedOmni));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarLimited));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarExpanded));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUPCA));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUPCE));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN13));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN8));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sGS1_128_CCA));
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sGS1_128_CCC));

	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sNONE));          // First
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNONE);
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sNUMSYMS - 1));   // Last
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sNUMSYMS - 1);
	TEST_CHECK(!gs1_encoder_setSym(ctx, gs1_encoder_sNONE - 1));     // Too small
	TEST_CHECK(!gs1_encoder_setSym(ctx, gs1_encoder_sNUMSYMS));      // Too big
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sUPCA));
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sUPCA);

	TEST_CHECK(gs1_encoder_sNUMSYMS == 14);  // Remember to add new symbologies

	gs1_encoder_free(ctx);

}


void test_api_fileInputFlag(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setFileInputFlag(ctx, true));     // dataFile
	TEST_CHECK(gs1_encoder_getFileInputFlag(ctx) == true);

	gs1_encoder_free(ctx);

}


void test_api_pixMult(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setPixMult(ctx, 1));
	TEST_CHECK(gs1_encoder_getPixMult(ctx) == 1);
	TEST_CHECK(!gs1_encoder_setPixMult(ctx, 0));
	TEST_CHECK(!gs1_encoder_setPixMult(ctx, MAX_PIXMULT + 1));
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


void test_api_XYundercut(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	// Special case
	gs1_encoder_setPixMult(ctx, 1);
	TEST_CHECK(gs1_encoder_setXundercut(ctx, 0));
	TEST_CHECK(gs1_encoder_setYundercut(ctx, 0));

	// Minima
	gs1_encoder_setPixMult(ctx, 2);
	TEST_CHECK(gs1_encoder_setXundercut(ctx, 1));
	TEST_CHECK(gs1_encoder_setYundercut(ctx, 1));

	// Maxima
	gs1_encoder_setPixMult(ctx, MAX_PIXMULT);
	TEST_CHECK(gs1_encoder_setXundercut(ctx, MAX_PIXMULT - 1));
	TEST_CHECK(gs1_encoder_setYundercut(ctx, MAX_PIXMULT - 1));

	// Must be less than X dimension
	gs1_encoder_setPixMult(ctx, 2);
	TEST_CHECK(!gs1_encoder_setXundercut(ctx, 2));
	TEST_CHECK(!gs1_encoder_setYundercut(ctx, 2));

	// Not negative
	TEST_CHECK(!gs1_encoder_setXundercut(ctx, -1));
	TEST_CHECK(!gs1_encoder_setYundercut(ctx, -1));

	gs1_encoder_free(ctx);

}


void test_api_sepHt(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

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
	gs1_encoder_setPixMult(ctx, MAX_PIXMULT);
	TEST_CHECK(gs1_encoder_setSepHt(ctx, MAX_PIXMULT));
	TEST_CHECK(!gs1_encoder_setSepHt(ctx, MAX_PIXMULT - 1));
	TEST_CHECK(gs1_encoder_setSepHt(ctx, 2 * MAX_PIXMULT));
	TEST_CHECK(!gs1_encoder_setSepHt(ctx, 2 * MAX_PIXMULT + 1));

	gs1_encoder_free(ctx);

}


void test_api_dmRowsColumns(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	// Default
	TEST_CHECK(gs1_encoder_getDmRows(ctx) == gs1_encoder_dmRowsAutomatic);
	TEST_CHECK(gs1_encoder_getDmColumns(ctx) == gs1_encoder_dmColumnsAutomatic);

	// Extents
	TEST_CHECK(gs1_encoder_setDmRows(ctx, gs1_encoder_dmRows8));
	TEST_CHECK(gs1_encoder_getDmRows(ctx) == gs1_encoder_dmRows8);
	TEST_CHECK(gs1_encoder_setDmRows(ctx, gs1_encoder_dmRows144));
	TEST_CHECK(gs1_encoder_getDmRows(ctx) == gs1_encoder_dmRows144);
	TEST_CHECK(gs1_encoder_setDmColumns(ctx, gs1_encoder_dmColumns10));
	TEST_CHECK(gs1_encoder_getDmColumns(ctx) == gs1_encoder_dmColumns10);
	TEST_CHECK(gs1_encoder_setDmColumns(ctx, gs1_encoder_dmColumns144));
	TEST_CHECK(gs1_encoder_getDmColumns(ctx) == gs1_encoder_dmColumns144);

	// Invalid
	TEST_CHECK(!gs1_encoder_setDmRows(ctx, gs1_encoder_dmRowsAutomatic - 1));
	TEST_CHECK(!gs1_encoder_setDmRows(ctx, gs1_encoder_dmRows8 - 1));
	TEST_CHECK(!gs1_encoder_setDmRows(ctx, gs1_encoder_dmRows144 + 1));
	TEST_CHECK(!gs1_encoder_setDmColumns(ctx, gs1_encoder_dmColumns10 - 1));
	TEST_CHECK(!gs1_encoder_setDmColumns(ctx, gs1_encoder_dmColumns144 + 1));

	// Back to automatic
	TEST_CHECK(gs1_encoder_setDmRows(ctx, gs1_encoder_dmRowsAutomatic));
	TEST_CHECK(gs1_encoder_getDmRows(ctx) == gs1_encoder_dmRowsAutomatic);
	TEST_CHECK(gs1_encoder_setDmColumns(ctx, gs1_encoder_dmColumnsAutomatic));
	TEST_CHECK(gs1_encoder_getDmColumns(ctx) == gs1_encoder_dmColumnsAutomatic);

	gs1_encoder_free(ctx);

}


void test_api_qrVersion(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_getQrVersion(ctx) == gs1_encoder_qrVersionAutomatic);  // Default

	// Extents
	TEST_CHECK(gs1_encoder_setQrVersion(ctx, gs1_encoder_qrVersion1));
	TEST_CHECK(gs1_encoder_getQrVersion(ctx) == gs1_encoder_qrVersion1);
	TEST_CHECK(gs1_encoder_setQrVersion(ctx, gs1_encoder_qrVersion40));
	TEST_CHECK(gs1_encoder_getQrVersion(ctx) == gs1_encoder_qrVersion40);

	// Invalid
	TEST_CHECK(!gs1_encoder_setQrVersion(ctx, gs1_encoder_qrVersionAutomatic - 1));
	TEST_CHECK(!gs1_encoder_setQrVersion(ctx, gs1_encoder_qrVersion40 + 1));

	// Back to automatic
	TEST_CHECK(gs1_encoder_setQrVersion(ctx, gs1_encoder_qrVersionAutomatic));
	TEST_CHECK(gs1_encoder_getQrVersion(ctx) == gs1_encoder_qrVersionAutomatic);

	gs1_encoder_free(ctx);

}


void test_api_qrEClevel(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_getQrEClevel(ctx) == gs1_encoder_qrEClevelM);  // Default

	TEST_CHECK(gs1_encoder_setQrEClevel(ctx, gs1_encoder_qrEClevelL));
	TEST_CHECK(gs1_encoder_getQrEClevel(ctx) == gs1_encoder_qrEClevelL);

	TEST_CHECK(gs1_encoder_setQrEClevel(ctx, gs1_encoder_qrEClevelM));
	TEST_CHECK(gs1_encoder_getQrEClevel(ctx) == gs1_encoder_qrEClevelM);

	TEST_CHECK(gs1_encoder_setQrEClevel(ctx, gs1_encoder_qrEClevelQ));
	TEST_CHECK(gs1_encoder_getQrEClevel(ctx) == gs1_encoder_qrEClevelQ);

	TEST_CHECK(gs1_encoder_setQrEClevel(ctx, gs1_encoder_qrEClevelH));
	TEST_CHECK(gs1_encoder_getQrEClevel(ctx) == gs1_encoder_qrEClevelH);

	TEST_CHECK(!gs1_encoder_setQrEClevel(ctx, gs1_encoder_qrEClevelL - 1));
	TEST_CHECK(!gs1_encoder_setQrEClevel(ctx, gs1_encoder_qrEClevelH + 1));

	gs1_encoder_free(ctx);

}


void test_api_addCheckDigit(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(!gs1_encoder_getAddCheckDigit(ctx));		// Default

	TEST_CHECK(gs1_encoder_setAddCheckDigit(ctx, true));	// Set
	TEST_CHECK(gs1_encoder_getAddCheckDigit(ctx));

	TEST_CHECK(gs1_encoder_setAddCheckDigit(ctx, false));	// Reset
	TEST_CHECK(!gs1_encoder_getAddCheckDigit(ctx));

	gs1_encoder_free(ctx);

}


void test_api_segWidth(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 6));
	TEST_CHECK(gs1_encoder_getDataBarExpandedSegmentsWidth(ctx) == 6);
	TEST_CHECK(!gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 0));
	TEST_CHECK(!gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 1));
	TEST_CHECK(gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 2));
	TEST_CHECK(!gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 5));    // not even
	TEST_CHECK(gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 22));
	TEST_CHECK(!gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 23));
	TEST_CHECK(!gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 24));

	gs1_encoder_free(ctx);

}


void test_api_linHeight(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setGS1_128LinearHeight(ctx, 12));
	TEST_CHECK(gs1_encoder_getGS1_128LinearHeight(ctx) == 12);
	TEST_CHECK(!gs1_encoder_setGS1_128LinearHeight(ctx, 0));
	TEST_CHECK(!gs1_encoder_setGS1_128LinearHeight(ctx, UCC128_MAX_LINHT+1));

	gs1_encoder_free(ctx);

}


void test_api_outFile(void) {

	gs1_encoder* ctx;

	char longfname[MAX_FNAME+2];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "a"));

	for (i = 0; i < MAX_FNAME; i++) {
		longfname[i]='a';
	}
	longfname[i+1]='\0';
	TEST_CHECK(!gs1_encoder_setOutFile(ctx, longfname));  // Too long

	longfname[i]='\0';
	TEST_CHECK(gs1_encoder_setOutFile(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


void test_api_format(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dTIF));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dBMP));       // reset filename
	TEST_CHECK(gs1_encoder_getFormat(ctx) == gs1_encoder_dBMP);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_BMP_FILE) == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dBMP));       // still bmp, no change
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dBMP));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dTIF));      // reset filename
	TEST_CHECK(gs1_encoder_getFormat(ctx) == gs1_encoder_dTIF);
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), DEFAULT_TIF_FILE) == 0);
	TEST_CHECK(gs1_encoder_setOutFile(ctx, "test.file"));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dTIF));      // still tiff, change
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "test.file") == 0);

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dBMP));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dTIF));      // now tif, don't reset empty filename
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "") == 0);

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dTIF));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dBMP));       // now bmp, don't reset empty filename
	TEST_CHECK(strcmp(gs1_encoder_getOutFile(ctx), "") == 0);

	gs1_encoder_free(ctx);

}


void test_api_dataFile(void) {

	gs1_encoder* ctx;

	char longfname[MAX_FNAME+2];
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setDataFile(ctx, "test.file"));
	TEST_CHECK(strcmp(gs1_encoder_getDataFile(ctx), "test.file") == 0);
	TEST_CHECK(!gs1_encoder_setDataFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setDataFile(ctx, "a"));

	for (i = 0; i <= MAX_FNAME; i++) {
		longfname[i]='a';
	}
	longfname[MAX_FNAME+1]='\0';
	TEST_CHECK(!gs1_encoder_setDataFile(ctx, longfname));  // Too long

	longfname[MAX_FNAME]='\0';
	TEST_CHECK(gs1_encoder_setDataFile(ctx, longfname));   // Maximun length

	gs1_encoder_free(ctx);

}


void test_api_dataStr(void) {

	gs1_encoder* ctx;
	int i;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_setDataStr(ctx, "barcode"));
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "barcode") == 0);
	TEST_CHECK(gs1_encoder_setDataStr(ctx, ""));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "a"));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "129912253123000123|#99123123"));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "#129912253123000123|#99123123"));

	for (i = 0; i <= MAX_DATA; i++) {
		bigbuffer[i]='a';
	}
	bigbuffer[MAX_DATA+1]='\0';
	TEST_CHECK(!gs1_encoder_setDataStr(ctx, bigbuffer));  // Too long

	bigbuffer[MAX_DATA]='\0';
	TEST_CHECK(gs1_encoder_setDataStr(ctx, bigbuffer));   // Maximun length

	gs1_encoder_free(ctx);

}


void test_api_getAIdataStr(void) {

	gs1_encoder* ctx;
	char *out;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_ASSERT(gs1_encoder_setDataStr(ctx, "#011231231231233310ABC123"));
	TEST_ASSERT((out = gs1_encoder_getAIdataStr(ctx)) != NULL);
	TEST_CHECK(strcmp(out, "(01)12312312312333(10)ABC123") == 0);

	TEST_ASSERT(gs1_encoder_setDataStr(ctx, "TESTING"));
	TEST_CHECK((out = gs1_encoder_getAIdataStr(ctx)) == NULL);

	// Escape data "(" characters
	TEST_ASSERT(gs1_encoder_setDataStr(ctx, "#10ABC(123"));
	TEST_ASSERT((out = gs1_encoder_getAIdataStr(ctx)) != NULL);
	TEST_CHECK(strcmp(out, "(10)ABC\\(123") == 0);

	// Composite strings
	TEST_ASSERT(gs1_encoder_setDataStr(ctx, "#011231231231233310ABC123|#99XYZ(TM)_CORP"));
	TEST_ASSERT((out = gs1_encoder_getAIdataStr(ctx)) != NULL);
	TEST_CHECK(strcmp(out, "(01)12312312312333(10)ABC123|(99)XYZ\\(TM)_CORP") == 0);

	gs1_encoder_free(ctx);

}


void test_api_getScanData(void) {

	gs1_encoder* ctx;
	char *out;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_ASSERT(gs1_encoder_setSym(ctx, gs1_encoder_sDataBarExpanded));
	TEST_ASSERT(gs1_encoder_setDataStr(ctx, "#011231231231233310ABC123#11991225|#98COMPOSITE#97XYZ"));
	TEST_ASSERT((out = gs1_encoder_getScanData(ctx)) != NULL);
	TEST_CHECK(strcmp(out, "]e0011231231231233310ABC123" "\x1D" "1199122598COMPOSITE" "\x1D" "97XYZ") == 0);

	gs1_encoder_free(ctx);

}


void test_api_setScanData(void) {

	gs1_encoder* ctx;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_ASSERT(gs1_encoder_setScanData(ctx, "]e0011231231231233310ABC123" "\x1D" "99XYZ"));
	TEST_CHECK(gs1_encoder_getSym(ctx) == gs1_encoder_sDataBarExpanded);
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "#011231231231233310ABC123#99XYZ") == 0);

	gs1_encoder_free(ctx);

}


void test_api_getHRI(void) {

	gs1_encoder* ctx;
	int numAIs;
	char **hri;

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_ASSERT(gs1_encoder_setDataStr(ctx, "#011231231231233310ABC123|#99COMPOSITE"));
	TEST_ASSERT((numAIs = gs1_encoder_getHRI(ctx, &hri)) == 4);
	TEST_ASSERT(hri != NULL);
	TEST_CHECK(strcmp(hri[0], "(01) 12312312312333") == 0);
	TEST_CHECK(strcmp(hri[1], "(10) ABC123") == 0);
	TEST_CHECK(strcmp(hri[2], "--") == 0);
	TEST_CHECK(strcmp(hri[3], "(99) COMPOSITE") == 0);

	gs1_encoder_free(ctx);

}


void test_api_getBuffer(void) {

	gs1_encoder* ctx;
	uint8_t* buf = NULL;
	size_t size;
	uint8_t test_tif[] = { 0x49, 0x49, 0x2A, 0x00 };
	uint8_t test_bmp[] = { 0x42, 0x4D, 0xDE, 0x04 };
	uint8_t test_raw[] = { 0x01, 0x49, 0xBD, 0x3A };

	TEST_ASSERT((ctx = gs1_encoder_init(NULL)) != NULL);

	TEST_CHECK(gs1_encoder_getBuffer(ctx, (void*)&buf) == 0);
	TEST_CHECK(gs1_encoder_getBufferWidth(ctx) == 0);
	TEST_CHECK(gs1_encoder_getBufferHeight(ctx) == 0);
	TEST_CHECK(buf == NULL);

	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN13));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "1234567890128"));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dTIF));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK((size = gs1_encoder_getBuffer(ctx, (void*)&buf)) == 1234);  // Really!
	TEST_CHECK(gs1_encoder_getBufferWidth(ctx) == 109);
	TEST_CHECK(gs1_encoder_getBufferHeight(ctx) == 74);
	assert(buf);
	TEST_CHECK(memcmp(buf, test_tif, sizeof(test_tif)) == 0);

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dBMP));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK((size = gs1_encoder_getBuffer(ctx, (void*)&buf)) == 1246);
	TEST_CHECK(gs1_encoder_getBufferWidth(ctx) == 109);
	TEST_CHECK(gs1_encoder_getBufferHeight(ctx) == 74);
	assert(buf);
	TEST_CHECK(memcmp(buf, test_bmp, sizeof(test_bmp)) == 0);

	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dRAW));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK((size = gs1_encoder_getBuffer(ctx, (void*)&buf)) == 1036);
	TEST_CHECK(gs1_encoder_getBufferWidth(ctx) == 109);
	TEST_CHECK(gs1_encoder_getBufferHeight(ctx) == 74);
	assert(buf);
	TEST_CHECK(memcmp(buf, test_raw, sizeof(test_raw)) == 0);

	// Check integrity of dataStr after encode
	TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sEAN13));
	TEST_CHECK(gs1_encoder_setDataStr(ctx, "1234567890128|#99123456"));
	TEST_CHECK(gs1_encoder_setOutFile(ctx, ""));
	TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dRAW));
	TEST_CHECK(gs1_encoder_encode(ctx));
	TEST_CHECK(strcmp(gs1_encoder_getDataStr(ctx), "1234567890128|#99123456") == 0);

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
