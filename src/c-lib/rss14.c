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

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "enc-private.h"
#include "driver.h"
#include "rss14.h"
#include "rssutil.h"
#include "cc.h"


// RSS14 Stacked row separator pattern routine
static struct sPrints *separator14S(gs1_encoder *ctx, struct sPrints *prints) {

	int i, j, k, lNdx, rNdx, sNdx, lWidth, rWidth, matchWidth;
	uint8_t *sepPattern = ctx->rss14_sepPattern;
	struct sPrints *prntSep = &ctx->rss14_prntSep;

	prntSep->leftPad = prints->leftPad;
	prntSep->rightPad = prints->rightPad;
	prntSep->reverse = prints->reverse;
	prntSep->pattern = sepPattern;
	prntSep->height = ctx->sepHt;
	prntSep->whtFirst = true;
	prntSep->guards = false;

	sepPattern[0] = sepPattern[1] = 1; // start with old SB guard in separator
	lNdx = 0; // left (top) element index
	rNdx = RSS14_ELMNTS/2; // right (bottom) element index
	sNdx = 2; // separator element index
	lWidth = rWidth = matchWidth = 0;
	for (i = 0; i < RSS14_SYM_W/2 - 2; i++, lWidth--, rWidth--) {
		if (lWidth == 0) {
			lWidth = prints->pattern[lNdx++]; // next left element width
		}
		if (rWidth == 0) {
			rWidth = prints->pattern[rNdx++]; // next left element width
		}
		if (((lNdx - rNdx) & 1) == 1) {
			// top and bottom rows are opposite colors here
			if (matchWidth > 0) {
				// same to opposite, terminate complimentary element
				sepPattern[sNdx++] = (uint8_t)matchWidth;
				matchWidth = 0;
			}
			sepPattern[sNdx++] = 1; // 1X elements separate opposite colors
		}
		else {
			if (matchWidth == 0) {
				// opposite to same
				if (((lNdx - sNdx) & 1) == 0) {
					// seperator is opposite color, start new element
					matchWidth = 1;
				}
				else {
					// wrong color, extend previous narrow to match color
					sNdx--;
					matchWidth = 2;
				}
			}
			else {
				// same to same, see if colors reversed
				if (((lNdx - sNdx) & 1) == 1) {
					// yes, terminate previous color
					sepPattern[sNdx++] = (uint8_t)matchWidth;
					matchWidth = 1;
				}
				else {
					// no, add to current color
					matchWidth++;
				}
			}
		}
	}
	sepPattern[sNdx] = sepPattern[sNdx+1] = 1; // old right guard pattern

	// insert 4X min space on each end
	for (i = k = 0; k <= 4; k += sepPattern[i], i++);
	if ((i&1)==0) {
		sepPattern[0] = 4;
		sepPattern[1] = (uint8_t)(k-4);
		j = 2;
	}
	else {
		sepPattern[0] = (uint8_t)k;
		j = 1;
	}
	for ( ; i < sNdx+2; i++, j++) {
		sepPattern[j] = sepPattern[i];
	}
	for (j--, k = 0; k <= 4; k += sepPattern[j], j--);
	if ((j&1)==0) {
		j += 2;
		sepPattern[j-1] = (uint8_t)(k-4);
		sepPattern[j] = 4;
	}
	else {
		j++;
		sepPattern[j] = (uint8_t)k;
	}
	prntSep->elmCnt = j+1;
	return(prntSep);
}


#define K	4
#define	PARITYCHRSIZE	9
#define PARITY_MOD 79

// left char multiplier
#define LEFT_MUL 4537077.

// outside semi-char multipliers
#define	SEMI_MUL	1597


// call with str = 13-digit primary, no check digit
static bool RSS14enc(gs1_encoder *ctx, uint8_t string[], uint8_t bars[], int ccFlag) {

	// stores even elements N & max, odd N & max, even mul, combos
	static const int tbl154[4*6] = {
		/* 15,4 */	10,7,	5,2,	4,	336,
				8,5,	7,4,	20,	700,
				6,3,	9,6,	48,	480,
				4,1,	11,8,	81,	81 };

	// stores odd elements N & max, even N & max, odd mul, combos
	static const int tbl164[5*6] = {
		/* 16,4 */	12,8,	4,1,	1,	161,
				10,6,	6,3,	10,	800,
				8,4,	8,5,	34,	1054,
				6,3,	10,6,	70,	700,
				4,1,	12,8,	126,126 };

	static const uint8_t leftParity[PARITYCHRSIZE * 3] = {
		3,8,2,
		3,5,5,
		3,3,7,
		3,1,9,
		2,7,4,
		2,5,6,
		2,3,8,
		1,5,7,
		1,3,9 };

	static const int leftWeights[4*K] = {
		1,3,9,27,2,6,18,54,4,12,36,29,8,24,72,58 };
	static const int rightWeights[4*K] = {
		16,48,65,37,32,17,51,74,64,34,23,69,49,68,46,59 };

	double data;
	int value;
	int i;
	int elementN, elementMax, parity;
	int leftPar, rightPar;
	long chrValue, chrValSave, semiValue, semiValSave, longNum;
	int iIndex;
	int *widths;

	data = atof((char*)string);
	if (ccFlag) data += 10000000000000.;

	bars[11] = 1; // init fixed patterns
	bars[12] = 1;
	bars[29] = 1;
	bars[30] = 1;

	// calculate left (high order) symbol half value:
	chrValue = chrValSave = (long)(data / LEFT_MUL);

	// determine the 1st (left) character
	// get the 1st char odd elements value
	semiValue = semiValSave = chrValue / SEMI_MUL;

	// get 1st char index into tbl164
	iIndex = 0;
	while (semiValue >= tbl164[iIndex+5]) {
		semiValue -= tbl164[iIndex+5];
		iIndex += 6;
	}

	// get odd elements N and max
	elementN = tbl164[iIndex];
	elementMax = tbl164[iIndex+1];
	longNum = value = (int)(semiValue / tbl164[iIndex+4]);

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 1);
	parity = 0l;
	for (i = 0; i < K; i++) {
		bars[(i * 2)] = (uint8_t)widths[i];
		parity += leftWeights[i * 2] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate even elements value:
	value = (int)(semiValue - (tbl164[iIndex+4] * longNum));
	elementN = tbl164[iIndex+2];
	elementMax = tbl164[iIndex+3];

	// generate and store even element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 0);
	for (i = 0; i < K; i++) {
		bars[1 + (i * 2)] = (uint8_t)widths[i];
		parity += leftWeights[(i * 2) + 1] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// get the 2nd char value
	semiValue = chrValue - (semiValSave * SEMI_MUL);

	// get 2nd char index into tbl154
	iIndex = 0;
	while (semiValue >= tbl154[iIndex+5]) {
		semiValue -= tbl154[iIndex+5];
		iIndex += 6;
	}

	// get even elements N and max
	elementN = tbl154[iIndex];
	elementMax = tbl154[iIndex+1];
	longNum = value = (int)(semiValue / tbl154[iIndex+4]);

	// generate and store even element widths of the 2nd char:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 1);
	for (i = 0; i < K; i++) {
		bars[19 - (i * 2)] = (uint8_t)widths[i];
		parity += leftWeights[(i * 2)+1+8] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate 2nd char odd elements value:
	value = (int)(semiValue - (tbl154[iIndex+4] * longNum));
	elementN = tbl154[iIndex+2];
	elementMax = tbl154[iIndex+3];

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 0);
	for (i = 0; i < K; i++) {
		bars[20 - (i * 2)] = (uint8_t)widths[i];
		parity += leftWeights[(i * 2)+8] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate right (low order) symbol half value:
	chrValue = (long)(data - ((double)chrValSave * LEFT_MUL));

	// determine the 3rd character
	// get the 3rd char odd elements value
	semiValue = semiValSave = chrValue / SEMI_MUL;

	// get 3rd char index into tbl164
	iIndex = 0;
	while (semiValue >= tbl164[iIndex+5]) {
		semiValue -= tbl164[iIndex+5];
		iIndex += 6;
	}

	// get odd elements N and max
	elementN = tbl164[iIndex];
	elementMax = tbl164[iIndex+1];
	longNum = value = (int)(semiValue / tbl164[iIndex+4]);

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 1);
	for (i = 0; i < K; i++) {
		bars[41 - (i * 2)] = (uint8_t)widths[i];
		parity += rightWeights[i * 2] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate even elements value:
	value = (int)(semiValue - (tbl164[iIndex+4] * longNum));
	elementN = tbl164[iIndex+2];
	elementMax = tbl164[iIndex+3];

	// generate and store even element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 0);
	for (i = 0; i < K; i++) {
		bars[40 - (i * 2)] = (uint8_t)widths[i];
		parity += rightWeights[(i * 2) + 1] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// get the 4th char value
	semiValue = chrValue - (semiValSave * SEMI_MUL);

	// get 4th char index into tbl154
	iIndex = 0;
	while (semiValue >= tbl154[iIndex+5]) {
		semiValue -= tbl154[iIndex+5];
		iIndex += 6;
	}

	// get even elements N and max
	elementN = tbl154[iIndex];
	elementMax = tbl154[iIndex+1];
	longNum = value = (int)(semiValue / tbl154[iIndex+4]);

	// generate and store even element widths of the 4th char:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 1);
	for (i = 0; i < K; i++) {
		bars[22 + (i * 2)] = (uint8_t)widths[i];
		parity += rightWeights[(i * 2)+1+8] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate 4th char odd elements value:
	value = (int)(semiValue - (tbl154[iIndex+4] * longNum));
	elementN = tbl154[iIndex+2];
	elementMax = tbl154[iIndex+3];

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, K, elementMax, 0);
	for (i = 0; i < K; i++) {
		bars[21 + (i * 2)] = (uint8_t)widths[i];
		parity += rightWeights[(i * 2)+8] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate finders
	if (parity >= 8) {
		parity++; // avoid 0,8 doppelganger
	}
	if (parity >= 72) {
		parity++; // avoid 8,0 doppelganger
	}
	leftPar = parity/9;
	rightPar = parity%9;
	// store left (high order) parity character in the bars:
	for (i = 0; i < 3; i++) {
		bars[8 + i] = leftParity[leftPar * 3 + i];
	}

	// store right (low order) parity character in the spaces:
	for (i = 0; i < 3; i++) {
		bars[33 - i] = leftParity[rightPar * 3 + i];
	}
	return(true);
}


void gs1_RSS14(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints *prntCnv;

	uint8_t linPattern[RSS14_ELMNTS];

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	char dataStr[GS1_ENCODERS_MAX_DATA+1];
	char primaryStr[14+1];
	char tempStr[28+1];

	int i;
	int rows, ccFlag;
	char *ccStr;
	int symHt;

	if (ctx->sym == gs1_encoder_sRSS14) {
		symHt = RSS14_SYM_H;
	}
	else {
		symHt = RSS14_TRNC_H;
	}
	strcpy(dataStr, ctx->dataStr);
	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(dataStr) > 13) {
		strcpy(ctx->errMsg, "primary data exceeds 13 digits");
		ctx->errFlag = true;
		return;
	}

	strcpy(tempStr, "000000000000");
	strcat(tempStr, dataStr);
	strcpy(primaryStr, tempStr + strlen(tempStr) - 13);

	if (!RSS14enc(ctx, (uint8_t*)primaryStr, linPattern, ccFlag) || ctx->errFlag) return;

#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < RSS14_ELMNTS; i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif
	ctx->line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = RSS14_ELMNTS;
	prints.pattern = linPattern;
	prints.guards = true;
	prints.height = ctx->pixMult*symHt;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if (!((rows = gs1_CC4enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) return;
#if PRNT
		{
			int j;
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < rows; i++) {
				for (j = 0; j < CCB4_ELMNTS; j++) {
					printf("%d", ccPattern[i][j]);
				}
				printf("\n");
			}
		}
#endif
		gs1_driverInit(ctx, ctx->pixMult*CCB4_WIDTH,
				ctx->pixMult*(rows*2+symHt) + ctx->sepHt);

		// Composite Component
		prints.elmCnt = CCB4_ELMNTS;
		prints.guards = false;
		prints.height = ctx->pixMult*2;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		prints.elmCnt = RSS14_ELMNTS;
		prints.pattern = linPattern;
		prints.guards = true;
		prints.height = ctx->pixMult*symHt;
		prints.leftPad = RSS14_L_PADR;

		// CC separator
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS-14
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*RSS14_SYM_W, ctx->pixMult*symHt);

		// RSS-14
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	return;
}


void gs1_RSS14S(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints *prntCnv;

	uint8_t linPattern[RSS14_ELMNTS];

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	char dataStr[GS1_ENCODERS_MAX_DATA+1];
	char primaryStr[14+1];
	char tempStr[28+1];

	int i;
	int rows, ccFlag;
	char *ccStr;

	strcpy(dataStr, ctx->dataStr);
	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(dataStr) > 13) {
		strcpy(ctx->errMsg, "primary data exceeds 13 digits");
		ctx->errFlag = true;
		return;
	}

	strcpy(tempStr, "000000000000");
	strcat(tempStr, dataStr);
	strcpy(primaryStr, tempStr + strlen(tempStr) - 13);

	if (!RSS14enc(ctx, (uint8_t*)primaryStr, linPattern, ccFlag) || ctx->errFlag) return;
#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < RSS14_ELMNTS; i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif
	ctx->line1 = true; // so first line is not Y undercut
	// init most common RSS14S row prints values
	prints.elmCnt = RSS14_ELMNTS/2;
	prints.guards = true;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if (!((rows = gs1_CC2enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) return;
#if PRNT
		{
			int j;
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < rows; i++) {
				for (j = 0; j < CCB2_ELMNTS; j++) {
					printf("%d", ccPattern[i][j]);
				}
				printf("\n");
			}
		}
#endif

		gs1_driverInit(ctx, ctx->pixMult*(CCB2_WIDTH),
				ctx->pixMult*(rows*2+RSS14_ROWS1_H+RSS14_ROWS2_H) + 2*ctx->sepHt);

		// Composite Component
		prints.elmCnt = CCB2_ELMNTS;
		prints.guards = false;
		prints.height = ctx->pixMult*2;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator
		prints.elmCnt = RSS14_ELMNTS/2;
		prints.pattern = linPattern;
		prints.rightPad = RSS14_R_PADR;
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS14S upper row
		prints.guards = true;
		prints.height = ctx->pixMult*RSS14_ROWS1_H;
		gs1_driverAddRow(ctx, &prints);

		// RSS14S separator pattern
		prntCnv = separator14S(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS14S lower row
		prints.height = ctx->pixMult*RSS14_ROWS2_H;
		prints.pattern = &linPattern[RSS14_ELMNTS/2];
		prints.whtFirst = false;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*(RSS14_SYM_W/2+2),
				ctx->pixMult*(RSS14_ROWS1_H+RSS14_ROWS2_H) + ctx->sepHt);

		// RSS14S upper row
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*RSS14_ROWS1_H;
		gs1_driverAddRow(ctx, &prints);

		// RSS14S separator pattern
		prntCnv = separator14S(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS14S lower row
		prints.pattern = &linPattern[RSS14_ELMNTS/2];
		prints.height = ctx->pixMult*RSS14_ROWS2_H;
		prints.whtFirst = false;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	return;
}


void gs1_RSS14SO(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints chexPrnts;
	struct sPrints *prntCnv;

	uint8_t linPattern[RSS14_ELMNTS];
	uint8_t chexPattern[RSS14_SYM_W/2+2];

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	char dataStr[GS1_ENCODERS_MAX_DATA+1];
	char primaryStr[14+1];
	char tempStr[28+1];

	int i;
	int rows, ccFlag;
	char *ccStr;

	for (i = 0; i < RSS14_SYM_W/2+2; i++) chexPattern[i] = 1; // chex = all 1X elements
	chexPattern[0] = 5; // wide space on left
	chexPattern[RSS14_SYM_W/2+1-7] = 4; // wide space on right
	chexPrnts.elmCnt = RSS14_SYM_W/2+2-7;
	chexPrnts.pattern = &chexPattern[0];
	chexPrnts.guards = false;
	chexPrnts.height = ctx->sepHt;
	chexPrnts.whtFirst = true;
	chexPrnts.leftPad = 0;
	chexPrnts.rightPad = 0; // assume not a composite for now
	chexPrnts.reverse = false;

	strcpy(dataStr, ctx->dataStr);
	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(dataStr) > 13) {
		strcpy(ctx->errMsg, "primary data exceeds 13 digits");
		ctx->errFlag = true;
		return;
	}

	strcpy(tempStr, "000000000000");
	strcat(tempStr, dataStr);
	strcpy(primaryStr, tempStr + strlen(tempStr) - 13);

	if (!RSS14enc(ctx, (uint8_t*)primaryStr, linPattern, ccFlag) || ctx->errFlag) return;
#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < RSS14_ELMNTS; i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif
	ctx->line1 = true; // so first line is not Y undercut
	// init most common RSS14SO row prints values
	prints.elmCnt = RSS14_ELMNTS/2;
	prints.guards = true;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		chexPrnts.rightPad = RSS14_R_PADR; // pad for composite
		if (!((rows = gs1_CC2enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) return;
#if PRNT
		{
			int j;
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < rows; i++) {
				for (j = 0; j < CCB2_ELMNTS; j++) {
					printf("%d", ccPattern[i][j]);
				}
				printf("\n");
			}
		}
#endif

		gs1_driverInit(ctx, ctx->pixMult*(CCB2_WIDTH),
			ctx->pixMult*(rows*2+RSS14_SYM_H*2) + 4*ctx->sepHt);

		// Composite Component
		prints.elmCnt = CCB2_ELMNTS;
		prints.guards = false;
		prints.height = ctx->pixMult*2;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator
		prints.elmCnt = RSS14_ELMNTS/2;
		prints.pattern = linPattern;
		prints.rightPad = RSS14_R_PADR;
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS14SO upper row
		prints.guards = true;
		prints.height = ctx->pixMult*RSS14_SYM_H;
		gs1_driverAddRow(ctx, &prints);

		// RSS14SO upper row separator pattern
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// chex pattern
		gs1_driverAddRow(ctx, &chexPrnts);

		// RSS14SO lower row separator pattern
		prints.pattern = &linPattern[RSS14_ELMNTS/2];
		prints.whtFirst = false;
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS14SO lower row
		prints.height = ctx->pixMult*RSS14_SYM_H;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*(RSS14_SYM_W/2+2),
			ctx->pixMult*(RSS14_SYM_H*2) + 3*ctx->sepHt);

		// RSS14SO upper row
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*RSS14_SYM_H;
		gs1_driverAddRow(ctx, &prints);

		// RSS14SO upper row separator pattern
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// chex pattern
		gs1_driverAddRow(ctx, &chexPrnts);

		// RSS14SO lower row separator pattern
		prints.pattern = &linPattern[RSS14_ELMNTS/2];
		prints.whtFirst = false;
		prntCnv = gs1_cnvSeparator(ctx, &prints);
		gs1_driverAddRow(ctx, prntCnv);

		// RSS14SO lower row
		prints.height = ctx->pixMult*RSS14_SYM_H;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	return;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_rss14_RSS14_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sRSS14, "2401234567890", expect));

	gs1_encoder_free(ctx);

}


void test_rss14_RSS14T_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sRSS14T, "2401234567890", expect));

	gs1_encoder_free(ctx);

}


void test_rss14_RSS14S_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
"    X XX X  X X X X     XXXXX X X  X X X X X      ",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sRSS14S, "2401234567890", expect));

	gs1_encoder_free(ctx);

}


void test_rss14_RSS14SO_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
" X X    X  X   XXX  XXXXX      X XXXX   X X  XX X ",
"    XXXX XX XXX   X      X X X  X    XXX X XX     ",
"     X X X X X X X X X X X X X X X X X X X X X    ",
"    X XX X     XX X     X X X   X  X     X X      ",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
"X XX X  X XXXXX  X XXXXX     XXX XX XXXXX X XXXX X",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sRSS14SO, "2401234567890", expect));

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */

