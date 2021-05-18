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
#include "cc.h"
#include "driver.h"
#include "rsslim.h"
#include "rssutil.h"

static struct sPrints *separatorLim(gs1_encoder *ctx, struct sPrints *prints) {

	int i, j, k;
	uint8_t *sepPattern = ctx->rsslim_sepPattern;
	struct sPrints *prntSep = &ctx->rsslim_prntSep;

	prntSep->leftPad = prints->leftPad;
	prntSep->rightPad = prints->rightPad;
	prntSep->reverse = prints->reverse;
	prntSep->pattern = sepPattern;
	prntSep->height = ctx->sepHt;
	prntSep->whtFirst = true;
	prntSep->guards = false;

	sepPattern[0] = sepPattern[1] = 1;
	sepPattern[RSSLIM_ELMNTS+2] = sepPattern[RSSLIM_ELMNTS+3] = 1;
	for (i = 0; i < RSSLIM_ELMNTS; i++) {
		sepPattern[i+2] = prints->pattern[i];
	}
	for (i = k = 0; k <= 4; k += sepPattern[i], i++);
	if ((i&1)==1) {
		sepPattern[0] = 4;
		sepPattern[1] = (uint8_t)(k-4);
		j = 2;
	}
	else {
		sepPattern[0] = (uint8_t)k;
		j = 1;
	}
	for ( ; i < RSSLIM_ELMNTS+4; i++, j++) {
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


#define	NN	26
#define	KK	7
#define PARITY_MOD 89
#define SUPL_VAL 2015133531096.

// left char multiplier
#define LEFT_MUL 2013571.

// call with str = 13-digit primary, no check digit
static bool RSSLimEnc(gs1_encoder *ctx, uint8_t string[], uint8_t bars[], int ccFlag) {

	// stores odd element N & max, even N & max, odd mul, combos
	static const long oddEvenTbl[1*7*6] = { /* 26,7 */
								17,6,	9,3,	28,		183064,
								13,5,	13,4,	728,	637000,
								9,3,	17,6,	6454,	180712,
								15,5,	11,4,	203,	490245,
								11,4,	15,5,	2408,	488824,
								19,8,	7,1,	1,		17094,
								7,1,	19,8,	16632,16632 };

	static const uint8_t parityPattern[PARITY_MOD * 14] = {
		 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 3, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 3, 1, 1, 1,
		 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 3, 2, 1, 1,
		 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 3, 1, 1, 1,
		 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 3, 1, 1, 1,
		 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 3, 1, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 3, 1, 1, 1,
		 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 3, 1, 1, 1,
		 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 3, 1, 1, 1,
		 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 3, 1, 1, 1,
		 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1,
		 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 1, 1,
		 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 3, 1, 1, 2, 1, 2, 1, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1,
		 1, 1, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1,
		 1, 1, 1, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1,
		 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1,
		 1, 2, 1, 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1,
		 1, 3, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 3, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 2, 1, 1,
		 1, 1, 1, 2, 1, 1, 1, 1, 3, 1, 1, 2, 1, 1,
		 1, 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 3, 1, 1,
		 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 1, 1,
		 1, 1, 1, 1, 1, 1, 2, 1, 1, 3, 2, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1,
		 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 2, 1, 1,
		 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1,
		 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1,
		 1, 1, 1, 2, 1, 2, 2, 1, 1, 1, 2, 1, 1, 1,
		 1, 1, 1, 3, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1,
		 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 2, 2, 1, 1,
		 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1,
		 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1,
		 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 3, 1, 1,
		 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 2, 1, 1,
		 1, 1, 1, 1, 2, 1, 1, 1, 1, 3, 2, 1, 1, 1,
		 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 2, 1, 1,
		 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 1, 1, 1,
		 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1,
		 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 2, 1, 1,
		 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1,
		 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1,
		 1, 2, 1, 1, 2, 2, 1, 1, 1, 1, 2, 1, 1, 1,
		 1, 2, 1, 2, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1,
		 1, 3, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1,
		 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1,
		 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1,
		 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1,
		 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1,
		 1, 1, 2, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 1,
		 1, 1, 2, 1, 1, 1, 1, 3, 1, 1, 2, 1, 1, 1,
		 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1,
		 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 2, 1, 1, 1,
		 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1,
		 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1,
		 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1,
		 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1,
		 2, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 1,
		 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 2, 1, 1, 1,
		 2, 1, 1, 1, 1, 2, 1, 1, 1, 2, 2, 1, 1, 1,
		 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1,
		 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1,
		 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1,
	 };

	static const int leftWeights[2*KK] = {1,3,9,27,81,65,17,51,64,14,42,37,22,66};
	static const int rightWeights[2*KK] = {20,60,2,6,18,54,73,41,34,13,39,28,84,74};

	double data;

	int value;
	int i;
	int elementN, elementMax, parity;
	long chrValue, chrValSave, longNum;
	int iIndex;
	int *widths;

	data = atof((char*)string);
	if (data > 1999999999999.) {
		return(false); // item number too large
	}
	if (ccFlag) data += SUPL_VAL;

	// calculate left (high order) symbol half value:
	chrValue = chrValSave = (long)(data / LEFT_MUL);

	// get 1st char index into oddEvenTbl
	iIndex = 0;
	while (chrValue >= oddEvenTbl[iIndex+5]) {
		chrValue -= oddEvenTbl[iIndex+5];
		iIndex += 6;
	}

	// get odd elements N and max
	elementN = (int)oddEvenTbl[iIndex];
	elementMax = (int)oddEvenTbl[iIndex+1];
	longNum = value = (int)(chrValue / oddEvenTbl[iIndex+4]);

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, KK, elementMax, 1);
	parity = 0l;
	for (i = 0; i < KK; i++) {
		bars[(i*2)] = (uint8_t)widths[i];
		parity += leftWeights[i * 2] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate even elements value:
	value = (int)(chrValue - (oddEvenTbl[iIndex+4] * longNum));
	elementN = (int)oddEvenTbl[iIndex+2];
	elementMax = (int)oddEvenTbl[iIndex+3];

	// generate and store even element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, KK, elementMax, 0);
	for (i = 0; i < KK; i++) {
		bars[(i*2)+1] = (uint8_t)widths[i];
		parity += leftWeights[(i * 2) + 1] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate right (low order) symbol half value:
	chrValue = (long)(data - ((double)chrValSave * LEFT_MUL));

	// get 2nd char index into oddEvenTbl
	iIndex = 0;
	while (chrValue >= oddEvenTbl[iIndex+5]) {
		chrValue -= oddEvenTbl[iIndex+5];
		iIndex += 6;
	}

	// get odd elements N and max
	elementN = (int)oddEvenTbl[iIndex];
	elementMax = (int)oddEvenTbl[iIndex+1];
	longNum = value = (int)(chrValue / oddEvenTbl[iIndex+4]);

	// generate and store odd element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, KK, elementMax, 1);
	for (i = 0; i < KK; i++) {
		bars[(i*2)+28] = (uint8_t)widths[i];
		parity += rightWeights[i * 2] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate even elements value:
	value = (int)(chrValue - (oddEvenTbl[iIndex+4] * longNum));
	elementN = (int)oddEvenTbl[iIndex+2];
	elementMax = (int)oddEvenTbl[iIndex+3];

	// generate and store even element widths:
	widths = gs1_getRSSwidths(ctx, value, elementN, KK, elementMax, 0);
	for (i = 0; i < KK; i++) {
		bars[(i*2)+1+28] = (uint8_t)widths[i];
		parity += rightWeights[(i * 2) + 1] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// store parity character in bars[]:
	for (i = 0; i < 14; i++) {
		bars[14 + i] =
					parityPattern[(parity * 14) + i];
	}
	return(true);
}


void gs1_RSSLim(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints *prntCnv;

	uint8_t linPattern[RSSLIM_ELMNTS];

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

	if (!RSSLimEnc(ctx, (uint8_t*)primaryStr, linPattern, ccFlag) || ctx->errFlag) return;
#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < RSSLIM_ELMNTS; i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif
	ctx->line1 = true; // so first line is not Y undercut
	// init most common RSS Limited row prints values
	prints.elmCnt = RSSLIM_ELMNTS;
	prints.pattern = linPattern;
	prints.height = ctx->pixMult*RSSLIM_SYM_H;
	prints.guards = true;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if (!((rows = gs1_CC3enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) return;
#if PRNT
		{
			int j;
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < rows; i++) {
				if (rows <= MAX_CCA3_ROWS) { // CCA composite
					for (j = 0; j < CCA3_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
				}
				else {
					for (j = 0; j < CCB3_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
				}
				printf("\n");
			}
		}
#endif

		if (rows <= MAX_CCA3_ROWS) { // CCA composite
			gs1_driverInit(ctx, ctx->pixMult*RSSLIM_SYM_W,
					ctx->pixMult*(rows*2+RSSLIM_SYM_H) + ctx->sepHt);

			// 2D composite
			prints.elmCnt = CCA3_ELMNTS;
			prints.guards = false;
			prints.height = ctx->pixMult*2;
			for (i = 0; i < rows; i++) {
				prints.pattern = ccPattern[i];
				gs1_driverAddRow(ctx, &prints);
			}

			prints.elmCnt = RSSLIM_ELMNTS;
			prints.pattern = linPattern;
			prints.height = ctx->pixMult*RSSLIM_SYM_H;
			prints.guards = true;

			// RSS Limited CC separator pattern
			prntCnv = separatorLim(ctx, &prints);
			gs1_driverAddRow(ctx, prntCnv);

			// RSS Limited row
			gs1_driverAddRow(ctx, &prints);

			gs1_driverFinalise(ctx);
		}
		else { // CCB composite, extends beyond RSS14L on left
			gs1_driverInit(ctx, ctx->pixMult*(RSSLIM_L_PADB+RSSLIM_SYM_W),
					ctx->pixMult*(rows*2+RSSLIM_SYM_H) + ctx->sepHt);

			// 2D composite
			prints.elmCnt = CCB3_ELMNTS;
			prints.guards = false;
			prints.height = ctx->pixMult*2;
			prints.leftPad = 0;
			for (i = 0; i < rows; i++) {
				prints.pattern = ccPattern[i];
				gs1_driverAddRow(ctx, &prints);
			}

			prints.elmCnt = RSSLIM_ELMNTS;
			prints.pattern = linPattern;
			prints.height = ctx->pixMult*RSSLIM_SYM_H;
			prints.guards = true;
			prints.leftPad = RSSLIM_L_PADB;

			// RSS Limited CC separator pattern
			prntCnv = separatorLim(ctx, &prints);
			gs1_driverAddRow(ctx, prntCnv);

			// RSS Limited row
			gs1_driverAddRow(ctx, &prints);

			gs1_driverFinalise(ctx);
		}
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*RSSLIM_SYM_W, ctx->pixMult*RSSLIM_SYM_H);

		// RSS Limited row
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	return;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_rsslim_RSSLIM_encode(void) {

        char** expect;

        gs1_encoder* ctx = gs1_encoder_init();

        expect = (char*[]){
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
"01000110011000110110101001110100101011010011010010010110001101110011001101",
NULL
        };
        TEST_CHECK(test_encode(ctx, gs1_encoder_sRSSLIM, "1501234567890", expect));

        gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
