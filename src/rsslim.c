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
#include "enc.h"
#include "util.h"
#include "rssutil.h"
#include "cc.h"

// not including guard bars
#define ELMNTS (46-4)
// symbol width in modules including any quiet zones:
#define SYM_W		74
// total pixel ht of RSS14L
#define SYM_H	10

#define L_PADB 10 // RSS14L left pad for ccb

static int RSSLimEnc(uint8_t str[], uint8_t pattern[], int ccFlag);
static struct sPrints *separatorLim(struct sParams *params, struct sPrints *prints);

extern int errFlag;
extern int line1;
extern uint8_t ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

void RSSLim(struct sParams *params) {

struct sPrints prints;
struct sPrints *prntCnv;

uint8_t linPattern[ELMNTS];

char primaryStr[14+1];
char tempStr[28+1];

int i;
int rows, ccFlag;
char *ccStr;

	ccStr = strchr(params->dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(params->dataStr) > 13) {
		errFlag = true;
		printf("\nprimary data exceeds 13 digits");
		return;
	}

	strcpy(tempStr, "000000000000");
	strcat(tempStr, params->dataStr);
	strcpy(primaryStr, tempStr + strlen(tempStr) - 13);

	if (RSSLimEnc((uint8_t*)primaryStr, linPattern, ccFlag)) {
		if (errFlag) {
			printf("\nRSS Limited encoding error occurred.");
			return;
		}
#if PRNT
		printf("\n%s", primaryStr);
		printf("\n");
		for (i = 0; i < ELMNTS; i++) {
			printf("%d", linPattern[i]);
		}
		printf("\n");
#endif
	}
	else {
		printf("\nerror occurred, exiting.");
		return;
	}
	line1 = true; // so first line is not Y undercut
	// init most common RSS Limited row prints values
	prints.elmCnt = ELMNTS;
	prints.pattern = linPattern;
	prints.height = params->pixMult*SYM_H;
	prints.guards = true;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if ((rows = CC3enc((uint8_t*)ccStr, ccPattern)) > 0) {
			if (errFlag) {
				printf("\nerror occurred, exiting.");
				return;
			}
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
		}

		if (params->bmp) {
			// note: BMP is bottom to top inverted
			if (rows <= MAX_CCA3_ROWS) { // CCA composite
				bmpHeader(params->pixMult*SYM_W,
						params->pixMult*(rows*2+SYM_H) + params->sepHt, params->outfp);

				// RSS Limited row
				printElmnts(params, &prints);

				// RSS Limited CC separator pattern
				prntCnv = separatorLim(params, &prints);
				printElmnts(params, prntCnv);

				// 2D composite
				prints.elmCnt = CCA3_ELMNTS;
				prints.guards = false;
				prints.height = params->pixMult*2;
				for (i = rows-1; i >= 0; i--) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}
			else { // CCB composite, extends beyond RSS14L on left
				bmpHeader(params->pixMult*(L_PADB+SYM_W),
						params->pixMult*(rows*2+SYM_H) + params->sepHt, params->outfp);

				// RSS Limited row
				prints.leftPad = L_PADB;
				printElmnts(params, &prints);

				// RSS Limited CC separator pattern
				prntCnv = separatorLim(params, &prints);
				printElmnts(params, prntCnv);

				// 2D composite
				prints.elmCnt = CCB3_ELMNTS;
				prints.guards = false;
				prints.height = params->pixMult*2;
				prints.leftPad = 0;
				for (i = rows-1; i >= 0; i--) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}
		}
		else { // TIF format
			if (rows <= MAX_CCA3_ROWS) { // CCA composite
				tifHeader(params->pixMult*SYM_W,
						params->pixMult*(rows*2+SYM_H) + params->sepHt, params->outfp);

          // 2D composite
				prints.elmCnt = CCA3_ELMNTS;
				prints.guards = false;
				prints.height = params->pixMult*2;
				for (i = 0; i < rows; i++) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}

				prints.elmCnt = ELMNTS;
				prints.pattern = linPattern;
				prints.height = params->pixMult*SYM_H;
				prints.guards = true;

				// RSS Limited CC separator pattern
				prntCnv = separatorLim(params, &prints);
				printElmnts(params, prntCnv);

				// RSS Limited row
				printElmnts(params, &prints);
			}
			else { // CCB composite, extends beyond RSS14L on left
				tifHeader(params->pixMult*(L_PADB+SYM_W),
						params->pixMult*(rows*2+SYM_H) + params->sepHt, params->outfp);

				// 2D composite
				prints.elmCnt = CCB3_ELMNTS;
				prints.guards = false;
				prints.height = params->pixMult*2;
				prints.leftPad = 0;
				for (i = 0; i < rows; i++) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}

				prints.elmCnt = ELMNTS;
				prints.pattern = linPattern;
				prints.height = params->pixMult*SYM_H;
				prints.guards = true;
				prints.leftPad = L_PADB;

				// RSS Limited CC separator pattern
				prntCnv = separatorLim(params, &prints);
				printElmnts(params, prntCnv);

				// RSS Limited row
				printElmnts(params, &prints);
			}
		}
	}
	else { // primary only
		if (params->bmp) {
			bmpHeader(params->pixMult*SYM_W, params->pixMult*SYM_H, params->outfp);
		}
		else {
			tifHeader(params->pixMult*SYM_W, params->pixMult*SYM_H, params->outfp);
		}

		// RSS Limited row
		printElmnts(params, &prints);
	}
	return;
}

// call with str = 13-digit primary, no check digit
static int RSSLimEnc(uint8_t string[], uint8_t bars[], int ccFlag) {

#define	N	26
#define	K	7
#define PARITY_MOD 89
#define SUPL_VAL 2015133531096.

// left char multiplier
#define LEFT_MUL 2013571.

	// stores odd element N & max, even N & max, odd mul, combos
	static long oddEvenTbl[1*7*6] = { /* 26,7 */
								17,6,	9,3,	28,		183064,
								13,5,	13,4,	728,	637000,
								9,3,	17,6,	6454,	180712,
								15,5,	11,4,	203,	490245,
								11,4,	15,5,	2408,	488824,
								19,8,	7,1,	1,		17094,
								7,1,	19,8,	16632,16632 };

	static uint8_t parityPattern[PARITY_MOD * 14] = {
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

	static int leftWeights[2*K] = {1,3,9,27,81,65,17,51,64,14,42,37,22,66};
	static int rightWeights[2*K] = {20,60,2,6,18,54,73,41,34,13,39,28,84,74};

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
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
	parity = 0l;
	for (i = 0; i < K; i++) {
		bars[(i*2)] = (uint8_t)widths[i];
		parity += leftWeights[i * 2] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate even elements value:
	value = (int)(chrValue - (oddEvenTbl[iIndex+4] * longNum));
	elementN = (int)oddEvenTbl[iIndex+2];
	elementMax = (int)oddEvenTbl[iIndex+3];

	// generate and store even element widths:
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
	for (i = 0; i < K; i++) {
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
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
	for (i = 0; i < K; i++) {
		bars[(i*2)+28] = (uint8_t)widths[i];
		parity += rightWeights[i * 2] * widths[i];
		parity = parity % PARITY_MOD;
	}

	// calculate even elements value:
	value = (int)(chrValue - (oddEvenTbl[iIndex+4] * longNum));
	elementN = (int)oddEvenTbl[iIndex+2];
	elementMax = (int)oddEvenTbl[iIndex+3];

	// generate and store even element widths:
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
	for (i = 0; i < K; i++) {
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

static struct sPrints prntSep;
static uint8_t sepPattern[SYM_W];

static struct sPrints *separatorLim(struct sParams *params, struct sPrints *prints) {
int i, j, k;

	prntSep.leftPad = prints->leftPad;
	prntSep.rightPad = prints->rightPad;
	prntSep.reverse = prints->reverse;
	prntSep.pattern = sepPattern;
	prntSep.height = params->sepHt;
	prntSep.whtFirst = true;
	prntSep.guards = false;

	sepPattern[0] = sepPattern[1] = 1;
	sepPattern[ELMNTS+2] = sepPattern[ELMNTS+3] = 1;
	for (i = 0; i < ELMNTS; i++) {
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
	for ( ; i < ELMNTS+4; i++, j++) {
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
	prntSep.elmCnt = j+1;
	return(&prntSep);
}
