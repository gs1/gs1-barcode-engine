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

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "assert.h"
#include "enc-private.h"
#include "cc.h"
#include "debug.h"
#include "driver.h"
#include "rssexp.h"
#include "rssutil.h"

// gets the next 12 bit sym char from bit string
static int getVal12(const uint8_t bitString[], const int symNdx) {
	int val, ndx;

	ndx = symNdx*3/2; // index into bitString
	if (symNdx & 1) {
		// odd sym char count, so val = 4+8 bits
		val = (int)bitString[ndx] & 0xF;
		val = (val << 8) + (int)bitString[ndx+1];
	}
	else {
		// even sym char count, so val = 8+4 bits
		val = (int)bitString[ndx];
		val = (val << 4) + ((int)bitString[ndx+1] >> 4);
	}
	return(val);
}


// looks for '#' (symbol separator) in string and returns char index iff found
static int isSymbolSepatator(const uint8_t string[]) {
	int i;

	for (i = 0; i < (int)strlen((char*)string); i++) {
		if (string[i] == '#') {  // Symbol separator
			return(i);
		}
	}
	return(0);
}


#define PARITY_MOD 211
#define PARITY_PWR 3
#define K	4

// Fills in elements for a symbol character given a element array and
// and a symbol char value. Updates the parity *weight. Will fill in
// the array forward or reverse order for odd or even characters.
// Returns the updated parity.
static int symCharPat(gs1_encoder *ctx, uint8_t bars[], int symValue, int parity, const int weight,
							 const int forwardFlag) {

	// odd elements N & max, even N & max, odd mul, combos:
	static const int tbl174[5*6] = {
			/* 17,4 */	12,7,	5,2,	4,	348,
					10,5,	7,4,	20,	1040,
					8,4,	9,5,	52,	1560,
					6,3,	11,6,	104,1040,
					4,1,	13,8,	204,204 };

	int i, value, saveVal;
	int elementN, elementMax;
	int wgtOdd, wgtEven;
	int iIndex;
	int *widths;

	wgtOdd = weight;
	wgtEven = (weight * 3) % PARITY_MOD;

	// get sym char val index into tbl174
	iIndex = 0;
	while (symValue >= tbl174[iIndex+5]) {
		symValue -= tbl174[iIndex+5];
		iIndex += 6;
	}

	// get odd elements N and max
	elementN = tbl174[iIndex];
	elementMax = tbl174[iIndex+1];
	saveVal = value = symValue / tbl174[iIndex+4];

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 0);
	for (i = 0; i < 4; i++) {
		if (forwardFlag) {
			bars[i*2] = (uint8_t)widths[i]; // store in 0,2,4,6
		}
		else {
			bars[7 - i*2] = (uint8_t)widths[i]; // else in 7,5,3,1
		}
		parity += wgtOdd * widths[i];
		parity = parity % PARITY_MOD;
		wgtOdd = (wgtOdd * 9) % PARITY_MOD;
	}

	// calculate even elements value:
	value = symValue - (tbl174[iIndex+4] * saveVal);
	elementN = tbl174[iIndex+2];
	elementMax = tbl174[iIndex+3];

	// generate and store even element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 1);
	for (i = 0; i < 4; i++) {
		if (forwardFlag) {
			bars[1 + i*2] = (uint8_t)widths[i]; // store in 1,3,5,7
		}
		else {
			bars[6 - i*2] = (uint8_t)widths[i]; // else in 6,4,2,0
		}
		parity += wgtEven * widths[i];
		parity = parity % PARITY_MOD;
		wgtEven = (wgtEven * 9) % PARITY_MOD;
	}
	return(parity);
}


#define FINDER_SIZE 6

// convert AI string to bar widths in dbl segments
static int RSS14Eenc(gs1_encoder *ctx, uint8_t string[], uint8_t bars[RSSEXP_MAX_DBL_SEGS][RSSEXP_ELMNTS], const int ccFlag) {

	static const uint8_t finders[FINDER_SIZE][3] = {
		{ 1,8,4 },
		{ 3,6,4 },
		{ 3,4,6 },
		{ 3,2,8 },
		{ 2,6,5 },
		{ 2,2,9 } };

	static const int finderSets[10][11] = {
		{ 1,	-1,	0,	 0,	0,	 0,	0,	 0,	0,	 0,	0},
		{ 1,	-2,	2,	 0,	0,	 0,	0,	 0,	0,	 0,	0},
		{ 1,	-3,	2,	-4,	0,	 0,	0,	 0,	0,	 0,	0},
		{ 1,	-5,	2,	-4,	3,	 0,	0,	 0,	0,	 0,	0},
		{ 1,	-5,	2,	-4,	4,	-6,	0,	 0,	0,	 0,	0},
		{ 1,	-5,	2,	-4,	5,	-6,	6,	 0,	0,	 0,	0},
		{ 1,	-1,	2,	-2,	3,	-3,	4,	-4,	0,	 0,	0},
		{ 1,	-1,	2,	-2,	3,	-3,	4,	-5,	5,	 0,	0},
		{ 1,	-1,	2,	-2,	3,	-3,	4,	-5,	6,	-6,	0},
		{ 1,	-1,	2,	-2,	3,	-4,	4,	-5,	5,	-6,	6} };

	// element 1 weighting for characters N determined by adjacent finder
	static const int parWts[24] = { 0,1,20,189,193,62,185,113,150,46,76,43,16,109,
					70,134,148,6,120,79,103,161,55,45 };

	int i, j;
	int parity, weight;
	int symValue;
	int size, fndrNdx, fndrSetNdx;
	uint8_t bitField[RSSEXP_MAX_DBL_SEGS*3];

	ctx->linFlag = true;
	parity = 0;
	weight = 0;

	if (((i=gs1_check2DData(string)) != 0) || ((i=isSymbolSepatator(string)) != 0)) {
		sprintf(ctx->errMsg, "illegal character in RSS Expanded data = '%c'", string[i]);
		ctx->errFlag = true;
		return(0);
	}

	gs1_putBits(ctx, bitField, 0, 1, (uint16_t)ccFlag); // 2D linkage bit
	size = gs1_pack(ctx, string, bitField);
	if (size < 0) {
		strcpy(ctx->errMsg, "data error");
		ctx->errFlag = true;
		return(0);
	}

	// note size is # of data chars, not segments
	if ((bitField[0]&0x40) == 0x40) {
		// method 1, insert variable length symbol bit field
		bitField[0] = (uint8_t)(bitField[0] | ((((size+1)&1)<<5) + ((size > 13)?0x10:0)));
	}
	if ((bitField[0]&0x60) == 0) {
		// method 00, insert variable length symbol bit field
		bitField[0] = (uint8_t)(bitField[0] | ((((size+1)&1)<<4) + ((size > 13)?8:0)));
	}
	if ((bitField[0]&0x71) == 0x30) {
		// method 01100/01101, insert variable length symbol bit field
		bitField[0] = (uint8_t)(bitField[0] | ((((size+1)&1)<<1) + ((size > 13)?1:0)));
	}
	fndrSetNdx = (size - 2) / 2;

	for (i = 0; i < (size+2)/2; i++) { // loop through all dbl segments
		fndrNdx = finderSets[fndrSetNdx][i];
		// fill left data char in dbl segment if not first (check char)
		j = (fndrNdx >= 0) ? fndrNdx*2 : -fndrNdx*2+1;
		if (i > 0) {
			weight = parWts[2*(j-2)];
			symValue = getVal12(bitField, i*2-1);
			parity = symCharPat(ctx, &bars[i][0], symValue, parity, weight, true);
		}
		// fill finder for dbl segment
		if (fndrNdx < 0) { // reversed finder
			bars[i][12] = finders[-fndrNdx-1][0];
			bars[i][11] = finders[-fndrNdx-1][1];
			bars[i][10] = finders[-fndrNdx-1][2];
			bars[i][9] = 1;
			bars[i][8] = 1;
		}
		else { // forward finder
			bars[i][8] = finders[fndrNdx-1][0];
			bars[i][9] = finders[fndrNdx-1][1];
			bars[i][10] = finders[fndrNdx-1][2];
			bars[i][11] = 1;
			bars[i][12] = 1;
		}
		// fill right data char in dbl segment if it exists
		if (size > i*2) {
			weight = parWts[2*(j-2)+1];
			symValue = getVal12(bitField, i*2);
			parity = symCharPat(ctx, &bars[i][8+5], symValue, parity, weight, false);
		}
	}
	// fill in first parity char
	symCharPat(ctx, bars[0], (size-3)*PARITY_MOD + parity, 0, weight, true);
	return(size+1);
}


void gs1_RSSExp(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints chexPrnts;
	struct sPrints *prntCnv;

	uint8_t linPattern[RSSEXP_MAX_DBL_SEGS*RSSEXP_ELMNTS+4];
	uint8_t chexPattern[RSSEXP_MAX_DBL_SEGS*RSSEXP_SYM_W+2];
	uint8_t dblPattern[RSSEXP_MAX_DBL_SEGS][RSSEXP_ELMNTS];

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	int i, j;
	int rows = 0, ccFlag = 0;
	int segs, lNdx, lNdx1, lMods, lHeight;
	int evenRow, rev;
	int chexSize;
	char *ccStr;
	int rPadl1, rPadcc;

	char *dataStr = ctx->dataStr;

	assert(ctx->dataBarExpandedSegmentsWidth >= 4 && ctx->dataBarExpandedSegmentsWidth <= 22);
	assert(ctx->dataBarExpandedSegmentsWidth % 2 == 0);

	DEBUG_PRINT("\nData: %s\n", dataStr);

	if (*dataStr != '^') {
		strcpy(ctx->errMsg, "primary data must be AI syntax (FNC1 in first position)");
		ctx->errFlag = true;
		return;
	}
	dataStr++;

	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
		DEBUG_PRINT("Primary %s\n", dataStr);
		DEBUG_PRINT("CC: %s\n", ccStr);
	}

	ctx->rssexp_rowWidth = ctx->dataBarExpandedSegmentsWidth; // save for getUnusedBitCnt
	if (!((segs = RSS14Eenc(ctx, (uint8_t*)dataStr, dblPattern, ccFlag)) > 0) || ctx->errFlag) goto out;

	lNdx = 0;
	for (i = 0; i < segs-1; i += 2) {
		for (j = 0; j < 8+5+8; j++) { // copy double segments
			linPattern[lNdx++] = dblPattern[i/2][j];
		}
	}
	if (i == segs-1) {
		for (j = 0; j < 8+5; j++) { // copy last odd segment if one exists
			linPattern[lNdx++] = dblPattern[i/2][j];
		}
	}
	j = (segs <= ctx->dataBarExpandedSegmentsWidth) ? segs : ctx->dataBarExpandedSegmentsWidth;
	i = (segs+j-1)/j; // number of linear rows
	lHeight = ctx->pixMult*i*RSSEXP_SYM_H + ctx->sepHt*(i-1)*3;
	lNdx = (j/2)*(8+5+8) + (j&1)*(8+5);
	lMods = 2 + (j/2)*(17+15+17) + (j&1)*(17+15) + 2;

	// set up checkered seperator pattern and print structure
	for (i = 0; i < RSSEXP_MAX_DBL_SEGS*RSSEXP_SYM_W+2; i++) {
		chexPattern[i] = 1; // chex = all 1X elements
	}
	chexPattern[0] = 5; // except first and last
	if ((lMods&1) == 0) {
		chexPattern[lMods-8] = 4;
		chexSize = lMods-7;
	}
	else {
		chexPattern[lMods-9] = 5;
		chexSize = lMods-8;
	}
	chexPrnts.elmCnt = chexSize;
	chexPrnts.pattern = &chexPattern[0];
	chexPrnts.guards = false;
	chexPrnts.height = ctx->sepHt;
	chexPrnts.whtFirst = true;
	chexPrnts.leftPad = 0;
	chexPrnts.rightPad = 0;
	chexPrnts.reverse = false;

	rPadcc = lMods - RSSEXP_L_PAD - CCB4_WIDTH;

	DEBUG_PRINT_PATTERN("Linear pattern", linPattern, lNdx);

	ctx->line1 = true; // so first line is not Y undercut

	if (ccFlag) {
		if (!((rows = gs1_CC4enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) goto out;

		DEBUG_PRINT_PATTERNS("CC pattern", (uint8_t*)(*ccPattern), CCB4_ELMNTS, rows);
	}

	if (ccFlag) {
		gs1_driverInit(ctx, (long)ctx->pixMult*(lMods),
				(long)ctx->pixMult*rows*2 + ctx->sepHt + lHeight);
	}
	else {
		gs1_driverInit(ctx, (long)ctx->pixMult*lMods, lHeight);
	}

	if (ccFlag) {
		// print composite component
		prints.elmCnt = CCB4_ELMNTS;
		prints.guards = false;
		prints.height = ctx->pixMult*2;
		prints.leftPad = RSSEXP_L_PAD;
		prints.rightPad = rPadcc;
		prints.whtFirst = true;
		prints.reverse = false;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}
	}

	// print RSS Exp
	evenRow = false; // start with 1st row
	prints.elmCnt = lNdx;
	prints.guards = true;
	prints.height = ctx->pixMult*RSSEXP_SYM_H;
	prints.leftPad = 0;
	prints.rightPad = 0;

	for (i = 0; i < segs-ctx->dataBarExpandedSegmentsWidth; i += ctx->dataBarExpandedSegmentsWidth) {

		rev = evenRow ^ ((i/2)&1);
		prints.pattern = &linPattern[(i/2)*(8+5+8)+(i&1)*8];
		prints.whtFirst = (i/2+1)&1;
		prints.reverse = rev;

		if (i > 0) {
			// chex pattern
			gs1_driverAddRow(ctx, &chexPrnts);
		}

		if ((i > 0) || (ccFlag)) {
			// CC or lower complement separator
			prntCnv = gs1_cnvSeparator(ctx, &prints);
			gs1_driverAddRow(ctx, prntCnv);
		}

		// upper RSS row
		gs1_driverAddRow(ctx, &prints);

		// upper complement separator
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		evenRow = !evenRow;
	}

	// print last or only RSS Expanded row
	lNdx1 = ((segs/2)*(8+5+8)+(segs&1)*(8+5)) - ((i/2)*(8+5+8)+(i&1)*8);
	rPadl1 = lMods - 4 -
		(((segs/2)*(17+15+17)+(segs&1)*(17+15)) - ((i/2)*(17+15+17)+(i&1)*17));
	prints.elmCnt = lNdx1;
	prints.pattern = &linPattern[(i/2)*(8+5+8)+(i&1)*8];
	prints.whtFirst = (i/2+1)&1;
	rev = evenRow ^ ((i/2)&1);
	if (rev && (((lNdx1-4)%8)&1)) {
		// can't reverse odd # finders so offset it right by one
		prints.leftPad = 1;
		prints.rightPad = rPadl1-1;
		prints.reverse = false;

		// chex pattern
		gs1_driverAddRow(ctx, &chexPrnts);

		// bottom complement separator
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// bottom right offset RSS E row
		gs1_driverAddRow(ctx, &prints);
	}
	else {
		// otherwise normal row
		prints.leftPad = 0;
		prints.rightPad = rPadl1;
		prints.reverse = rev;

		if (i > 0) {
			// chex pattern
			gs1_driverAddRow(ctx, &chexPrnts);
		}

		if ((i > 0) || (ccFlag)) {
			// CC or lower complement separator
			prntCnv = gs1_cnvSeparator(ctx, &prints);
			gs1_driverAddRow(ctx, prntCnv);
		}

		// bottom right offset RSS E row
		gs1_driverAddRow(ctx, &prints);
	}

	gs1_driverFinalise(ctx);

out:

	// Restore the original dataStr contents
	if (ccFlag)
		*(ccStr-1) = '|';

	return;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_rssexp_RSSEXP_encode(void) {

	const char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (const char*[]){
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X ",
NULL
	};
	TEST_CHECK(test_encode(ctx, true, gs1_encoder_sDataBarExpanded, "^01950123456789033103000123", expect));

	gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, 4);
	expect = (const char*[]){
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
" X X   X   XXXX   X XXXXXXXX    X X XXX     XX   X XXX   XX   X  XX X XXXX      XXX  X XXX   XXX XXX X",
"    XXX XXX    XXX X        X X  X X   XXXXX  XXX X   XXX  XXX XX  X X    X X X    XX X   XXX   X     ",
"     X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X    ",
"        X X    XXX  X X      X X  X XXX XX XXXXXX                                                     ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
"  X XXXX X XXXX   XX   XXXXXX    X X   X  X      XX X                                                 ",
NULL
	};
	TEST_CHECK(test_encode(ctx, true, gs1_encoder_sDataBarExpanded, "^01950123456789033103000123", expect));

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */

