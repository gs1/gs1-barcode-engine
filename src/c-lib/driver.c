/**
 * GS1 Barcode Engine
 *
 * @author Copyright (c) 2000-2021 GS1 AISBL.
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
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "enc-private.h"
#include "driver.h"


static bool emitData(gs1_encoder *ctx, const void *data, const size_t len) {

	uint8_t *buf;

	if (strcmp(ctx->outFile, "") != 0) {
		fwrite(data, len, 1, ctx->outfp);
	} else {
		if (ctx->bufferSize + len > ctx-> bufferCap) {
			if ((buf = realloc(ctx->buffer, ctx->bufferCap * 2)) == NULL) {
				free(ctx->buffer);
				ctx->bufferCap = 0;
				ctx->bufferSize = 0;
				ctx->bufferWidth = 0;
				ctx->bufferHeight = 0;
				strcpy(ctx->errMsg, "Failed to expand output buffer");
				ctx->errFlag = true;
				return false;
			};
			ctx->buffer = buf;
			ctx->bufferCap *= 2;
		}
		memcpy(&ctx->buffer[ctx->bufferSize], data, len);
		ctx->bufferSize += len;
	}
	return true;

}


static void bmpHeader(gs1_encoder *ctx, const long xdim, const long ydim) {

	uint8_t id[2] = {'B','M'};
	struct b_hdr {
		uint32_t fileLength;
		uint32_t const0a;
		uint32_t const3E;
		uint32_t const28;
		uint32_t width;
		uint32_t height;
		uint16_t const1a;
		uint16_t const1b;
		uint32_t const0b;
		uint32_t const0c;
		uint32_t const0d;
		uint32_t const0e;
		uint32_t const0f;
		uint32_t const0g;
		uint32_t const0h;
		uint32_t constFFFFFF;
	};

	struct b_hdr header = { 0,0,0x3E,0x28,0,0,1,1,
				0,0,0,0,0,0,0,0xFFFFFF };

	header.width = (uint32_t)xdim;
	header.height = (uint32_t)ydim;
	header.fileLength = (uint32_t)(0x3E + (((xdim+31)/32)*4) * ydim); // pad rows to 32-bit boundary

	emitData(ctx, &id, sizeof(id));
	emitData(ctx, &header, sizeof(header));
}


static void tifHeader(gs1_encoder *ctx, const long xdim, const long ydim) {

	struct t_hdr {
		uint8_t endian[2];
		uint16_t version;
		uint32_t diroff;
	};

	struct t_tag {
		uint16_t tag_type;
		uint16_t num_size;
		uint32_t length;
		uint32_t offset;
	};

#define TAG_CNT 14

	struct t_hdr header = { {'I','I'},42,8L };
	short tagnum = TAG_CNT;
	struct t_tag type = { 0xFE, 4, 1L, 0L };
	struct t_tag width = { 0x100, 3, 1L, 6L };
	struct t_tag height = { 0x101 ,3 ,1L, 16L };
	struct t_tag bitsPerSample = { 0x102, 3, 1L, 1L };
	struct t_tag compress = { 0x103, 3, 1L, 1L };
	struct t_tag whiteIs = { 0x106, 3, 1L, 0L };
	struct t_tag thresholding = { 0x107, 3, 1L, 1L };
	struct t_tag stripOffset = { 0x111, 4, 1L, 8L+2+TAG_CNT*12+4+8+8 };
	struct t_tag samplesPerPix = { 0x115, 3, 1L, 1L };
	struct t_tag stripRows = { 0x116, 4, 1L, 16L };
	struct t_tag stripBytes = { 0x117, 4, 1L, 16L };
	struct t_tag xRes = { 0x11A, 5, 1L, 8L+2+TAG_CNT*12+4 };
	struct t_tag yRes = { 0x11B, 5, 1L, 8L+2+TAG_CNT*12+4+8 };
	struct t_tag resUnit = { 0x128, 3, 1L, 3L }; // centimeters
	uint32_t nextdir = 0L;
	uint32_t xResData[2] = { 120L, 1L }; // 120 = 10mils @ 300 dpi
	uint32_t yResData[2] = { 120L, 1L }; // 120 = 10mils @ 300 dpi

	width.offset = (uint32_t)xdim;
	height.offset = (uint32_t)ydim;
	stripRows.offset = (uint32_t)ydim;
	stripBytes.offset = (uint32_t)(((xdim+7)/8) * ydim);
	xResData[1] = 1L; //reduce to 10 mils
	yResData[1] = 1L; //reduce to 10 mils

	emitData(ctx, &header, sizeof(header));
	emitData(ctx, &tagnum, sizeof(tagnum));
	emitData(ctx, &type, sizeof(type));
	emitData(ctx, &width, sizeof(width));
	emitData(ctx, &height, sizeof(height));
	emitData(ctx, &bitsPerSample, sizeof(bitsPerSample));
	emitData(ctx, &compress, sizeof(compress));
	emitData(ctx, &whiteIs, sizeof(whiteIs));
	emitData(ctx, &thresholding, sizeof(thresholding));
	emitData(ctx, &stripOffset, sizeof(stripOffset));
	emitData(ctx, &samplesPerPix, sizeof(samplesPerPix));
	emitData(ctx, &stripRows, sizeof(stripRows));
	emitData(ctx, &stripBytes, sizeof(stripBytes));
	emitData(ctx, &xRes, sizeof(xRes));
	emitData(ctx, &yRes, sizeof(yRes));
	emitData(ctx, &resUnit, sizeof(resUnit));
	emitData(ctx, &nextdir, sizeof(nextdir));
	emitData(ctx, &xResData, sizeof(xResData));
	emitData(ctx, &yResData, sizeof(yResData));
	return;
}


static void printElm(gs1_encoder *ctx, const int width, const int color, int *bits, int *ndx, const uint8_t xorMsk) {

	int i;
	uint8_t *line = ctx->driver_line;
	uint8_t *lineUCut = ctx->driver_lineUCut;

	for (i = 0; i < width; i++) {
		*bits = (*bits<<1) + color;
		if (*bits > 0xff) {
			lineUCut[*ndx] = (uint8_t)(((line[*ndx]^xorMsk)&(*bits&0xff))^xorMsk); // Y undercut
			line[(*ndx)++] = (uint8_t)((*bits&0xff) ^ xorMsk);
			if (*ndx >= MAX_LINE/8 + 1) {
				*ndx = 0;
				strcpy(ctx->errMsg, "Print line too long in graphic line.");
				ctx->errFlag = true;
				return;
			}
			*bits = 1;
		}
	}
	return;
}


#define WHITE 0

static void printElmnts(gs1_encoder *ctx, const struct sPrints *prints) {

	int i, bits, width, ndx, white;
	uint8_t xorMsk;
	int undercut;
	uint8_t *line = ctx->driver_line;
	uint8_t *lineUCut = ctx->driver_lineUCut;

	bits = 1;
	ndx = 0;
	if (prints->whtFirst) {
		white = WHITE;
		undercut = ctx->Xundercut;
	}
	else {
		white = WHITE^1; // invert white if starting black
		undercut = -ctx->Xundercut; // -undercut if starting black
	}
	if ((prints->reverse) && ((prints->elmCnt & 1) == 0)) {
		white = white^1; // invert if reversed even elements
		undercut = -undercut;
	}
	xorMsk = ctx->format == gs1_encoder_dBMP ? 0xFF : 0; // invert BMP bits
	if (ctx->line1) {
		for (i = 0; i < MAX_LINE/8; i++) {
			line[i] = xorMsk;
		}
		ctx->line1 = false;
	}
	// fill left pad worth of WHITE
	printElm(ctx, prints->leftPad*ctx->pixMult, WHITE , &bits, &ndx, xorMsk);

	// process WHITE/BLACK elements in pairs for undercut
	if (prints->guards) { // print guard pattern
		printElm(ctx, ctx->pixMult + undercut, white , &bits, &ndx, xorMsk);
		printElm(ctx, ctx->pixMult - undercut, (white^1) , &bits, &ndx, xorMsk);
	}
	for(i = 0; i < prints->elmCnt-1; i += 2) {
		if (prints->reverse) {
			width = (int)prints->pattern[prints->elmCnt-1-i]*ctx->pixMult + undercut;
		}
		else {
			width = (int)prints->pattern[i]*ctx->pixMult + undercut;
		}
		printElm(ctx, width, white , &bits, &ndx, xorMsk);

		if (prints->reverse) {
			width = (int)prints->pattern[prints->elmCnt-2-i]*ctx->pixMult - undercut;
		}
		else {
			width = (int)prints->pattern[i+1]*ctx->pixMult - undercut;
		}
		printElm(ctx, width, (white^1) , &bits, &ndx, xorMsk);
	}

	// process any trailing odd numbered element with no undercut
	if (i < prints->elmCnt) {
		if (prints->guards) { // print last element plus guard pattern
			if (prints->reverse) {
				width = (int)prints->pattern[0]*ctx->pixMult + undercut;
			}
			else {
				width = (int)prints->pattern[i]*ctx->pixMult + undercut;
			}
			printElm(ctx, width, white , &bits, &ndx, xorMsk);

			printElm(ctx, ctx->pixMult - undercut, (white^1) , &bits, &ndx, xorMsk);
			printElm(ctx, ctx->pixMult, white , &bits, &ndx, xorMsk); // last- no undercut
		}
		else { // no guard, print last odd without undercut
			if (prints->reverse) {
				width = (int)prints->pattern[0]*ctx->pixMult;
			}
			else {
				width = (int)prints->pattern[i]*ctx->pixMult;
			}
			printElm(ctx, width, white , &bits, &ndx, xorMsk);
		}
	}
	else if (prints->guards) { // even number, just print guard pattern
		printElm(ctx, ctx->pixMult + undercut, white , &bits, &ndx, xorMsk);
		printElm(ctx, ctx->pixMult - undercut, (white^1) , &bits, &ndx, xorMsk);
	}
	// fill right pad worth of WHITE
	printElm(ctx, prints->rightPad*ctx->pixMult, WHITE , &bits, &ndx, xorMsk);
	// pad last byte's bits
	if (bits != 1) {
		while ((bits = (bits<<1) + WHITE) <= 0xff);
		lineUCut[ndx] = (uint8_t)(((line[ndx]^xorMsk)&(bits&0xff))^xorMsk); // Y undercut
		line[ndx++] = (uint8_t)((bits&0xff) ^ xorMsk);
		if (ndx > MAX_LINE/8 + 1) {
			strcpy(ctx->errMsg, "Print line too long");
			ctx->errFlag = true;
			return;
		}
	}
	if (ctx->format == gs1_encoder_dBMP) {
		while ((ndx & 3) != 0) {
			line[ndx++] = 0xFF; // pad to long word boundary for .BMP
			if (ndx >= MAX_LINE/8 + 1) {
				strcpy(ctx->errMsg, "Print line too long");
				ctx->errFlag = true;
				return;
			}
		}
	}

	for (i = 0; i < ctx->Yundercut; i++) {
		emitData(ctx, lineUCut, (size_t)ndx * sizeof(uint8_t));
	}
	for ( ; i < prints->height; i++) {
		emitData(ctx, line, (size_t)ndx * sizeof(uint8_t));
	}
	return;
}


bool gs1_doDriverInit(gs1_encoder *ctx, const long xdim, const long ydim) {

	FILE* oFile;

	if (strcmp(ctx->outFile, "") != 0) {
		if ((oFile = fopen(ctx->outFile, "wb")) == NULL) {
			sprintf(ctx->errMsg, "Unable to open file: %s", ctx->outFile);
			ctx->errFlag = true;
			return false;
		}
		ctx->outfp = oFile;
	} else {
		ctx->bufferCap = 1024;		// Initial size, will grow as needed
		if ((ctx->buffer = malloc(ctx->bufferCap * sizeof(uint8_t))) == NULL) {
			ctx->bufferCap = 0;
			strcpy(ctx->errMsg, "Out of memory allocating output buffer");
			ctx->errFlag = true;
			return false;
		}
		ctx->bufferSize = 0;
		ctx->bufferWidth = (int)xdim;
		ctx->bufferHeight = (int)ydim;
	}

	if (ctx->format == gs1_encoder_dBMP) {
		if ((ctx->driver_rowBuffer = malloc((unsigned long)ydim * sizeof(struct sPrints))) == NULL) {
			strcpy(ctx->errMsg, "Out of memory creating initial BMP row buffer");
			ctx->errFlag = true;
			return false;
		}
		ctx->driver_numRows = 0;

		bmpHeader(ctx, xdim, ydim);
	} else if (ctx->format == gs1_encoder_dTIF) {
		tifHeader(ctx, xdim, ydim);
	}

	return true;

}


bool gs1_doDriverAddRow(gs1_encoder *ctx, const struct sPrints *prints) {

	struct sPrints *row;

	if (ctx->format == gs1_encoder_dBMP) {

		// Buffer the row and its pattern
		row = &ctx->driver_rowBuffer[ctx->driver_numRows++];
		memcpy(row, prints, sizeof(struct sPrints));
		if ((row->pattern = malloc((unsigned int)prints->elmCnt * sizeof(uint8_t))) == NULL) {
			strcpy(ctx->errMsg, "Out of memory extending BMP row buffer");
			ctx->errFlag = true;
			return false;
		}
		memcpy(row->pattern, prints->pattern, (unsigned int)prints->elmCnt * sizeof(uint8_t));

	} else {  // TIF and RAW
		// Directly emit the row
		printElmnts(ctx, prints);
	}

	return true;

}


bool gs1_doDriverFinalise(gs1_encoder *ctx) {

	uint8_t* buf;
	int i;

	if (ctx->format == gs1_encoder_dBMP) {
		// Emit the rows in reverse, releasing their patterns
		for (i = ctx->driver_numRows - 1; i >= 0; i--) {
			printElmnts(ctx, &ctx->driver_rowBuffer[i]);
			free(ctx->driver_rowBuffer[i].pattern);
		}

		free(ctx->driver_rowBuffer);
		ctx->driver_rowBuffer = NULL;
	}

	if (strcmp(ctx->outFile, "") != 0) {
		fclose(ctx->outfp);
	} else {
		// Shrink the buffer to fit the data
		if ((buf = realloc(ctx->buffer, ctx->bufferSize * sizeof(uint8_t))) == NULL) {
			free(ctx->buffer);
			ctx->bufferCap = 0;
			ctx->bufferSize = 0;
			strcpy(ctx->errMsg, "Failed to shrink output buffer");
			ctx->errFlag = true;
			return false;
		};
		ctx->buffer = buf;
		ctx->bufferCap = ctx->bufferSize;
	}

	return true;

}


// Find pixMult that produces X dimension closest to target, within optional constraints
static int findPixMultForConstraints(gs1_encoder *ctx) {

	double diff, best = DBL_MAX;
	int i, pixMult = 0;

	assert(ctx);
	assert(ctx->deviceRes > 0);
	assert(ctx->minX == 0 || ctx->maxX == 0 || ctx->minX <= ctx->maxX);
	assert(ctx->minX == 0 || ctx->targetX >= ctx->minX);
	assert(ctx->maxX == 0 || ctx->targetX <= ctx->maxX);

	for (i = 1; i <= MAX_PIXMULT; i++) {
		if (ctx->minX != 0 && ((double)i)/ctx->deviceRes < ctx->minX)
			continue;
		if (ctx->maxX != 0 && ((double)i)/ctx->deviceRes > ctx->maxX)
			continue;
		diff = fabs(((double)i)/ctx->deviceRes - ctx->targetX);
		if (diff < best - 0.00001) {		// Handle roundoff
			best = diff;
			pixMult = i;
		}
	}

	if (!pixMult) {
		sprintf(ctx->errMsg, "Impossible to plot X-dimension of %.4f units within the range %.4f - %.4f units at resolution of %g dots per unit", ctx->targetX, ctx->minX, ctx->maxX, ctx->deviceRes);
		ctx->errFlag = true;
		return 0;
	}

	return pixMult;

}


bool gs1_setXdimension(gs1_encoder *ctx, const double minX, const double targetX, const double maxX) {

	int pixMult;

	if (ctx->deviceRes == 0) {
		strcpy(ctx->errMsg, "Must set device resolution when specifying X-dimension constraints");
		ctx->errFlag = true;
		goto fail;
	}
	if (minX < 0) {
		strcpy(ctx->errMsg, "Minimum X-dimension cannot be negative");
		ctx->errFlag = true;
		goto fail;
	}
	if (targetX < 0) {
		strcpy(ctx->errMsg, "Target X-dimension must be positive");
		ctx->errFlag = true;
		goto fail;
	}
	if (maxX < 0) {
		strcpy(ctx->errMsg, "Maximum X-dimension cannot be negative");
		ctx->errFlag = true;
		goto fail;
	}
	if (minX != 0 && maxX != 0 && maxX < minX) {
		strcpy(ctx->errMsg, "Minimum X-dimension cannot be greater than maximum X-dimension");
		ctx->errFlag = true;
		goto fail;
	}
	if ((minX != 0 && targetX < minX) ||
	    (maxX != 0 && targetX > maxX)) {
		strcpy(ctx->errMsg, "Target X-dimension must not be outside the specified minimum and maximum");
		ctx->errFlag = true;
		goto fail;
	}

	ctx->minX = minX;
	ctx->targetX = targetX;
	ctx->maxX = maxX;

	if ((pixMult = findPixMultForConstraints(ctx)) == 0)
		goto fail;	// Error already set

	ctx->pixMult = pixMult;
	if (pixMult <= ctx->Xundercut)
		ctx->Xundercut = 0;
	if (pixMult <= ctx->Yundercut)
		ctx->Yundercut = 0;
	if (pixMult * 2 < ctx->sepHt || pixMult > ctx->sepHt)
		ctx->sepHt = pixMult;

	return true;

fail:

	ctx->pixMult = 0;	// Disable rendering until resolved by user
	return false;

}
