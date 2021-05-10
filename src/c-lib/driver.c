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
#include <stdint.h>
#include <string.h>

#include "enc-private.h"
#include "driver.h"


void gs1_bmpHeader(long xdim, long ydim, FILE *oFile) {

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

	fwrite(&id, sizeof(id), 1, oFile);
	fwrite(&header, sizeof(header), 1, oFile);
}


void gs1_tifHeader(long xdim, long ydim, FILE *oFile) {

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

	fwrite(&header, sizeof(header), 1, oFile);
	fwrite(&tagnum, sizeof(tagnum), 1, oFile);
	fwrite(&type, sizeof(type), 1, oFile);
	fwrite(&width, sizeof(width), 1, oFile);
	fwrite(&height, sizeof(height), 1, oFile);
	fwrite(&bitsPerSample, sizeof(bitsPerSample), 1, oFile);
	fwrite(&compress, sizeof(compress), 1, oFile);
	fwrite(&whiteIs, sizeof(whiteIs), 1, oFile);
	fwrite(&thresholding, sizeof(thresholding), 1, oFile);
	fwrite(&stripOffset, sizeof(stripOffset), 1, oFile);
	fwrite(&samplesPerPix, sizeof(samplesPerPix), 1, oFile);
	fwrite(&stripRows, sizeof(stripRows), 1, oFile);
	fwrite(&stripBytes, sizeof(stripBytes), 1, oFile);
	fwrite(&xRes, sizeof(xRes), 1, oFile);
	fwrite(&yRes, sizeof(yRes), 1, oFile);
	fwrite(&resUnit, sizeof(resUnit), 1, oFile);
	fwrite(&nextdir, sizeof(nextdir), 1, oFile);
	fwrite(&xResData, sizeof(xResData), 1, oFile);
	fwrite(&yResData, sizeof(yResData), 1, oFile);
	return;
}


static void printElm(gs1_encoder *ctx, int width, int color, int *bits, int *ndx, uint8_t xorMsk) {

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

void gs1_printElmnts(gs1_encoder *ctx, struct sPrints *prints) {

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
	xorMsk = ctx->bmp ? 0xFF : 0; // invert BMP bits
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
	if (ctx->bmp) {
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
		fwrite(lineUCut, sizeof(uint8_t), (size_t)ndx, ctx->outfp);
	}
	for ( ; i < prints->height; i++) {
		fwrite(line, sizeof(uint8_t), (size_t)ndx, ctx->outfp);
	}
	return;
}
