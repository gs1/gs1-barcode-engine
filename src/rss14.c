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
// symbol width in modules including guard bars:
#define SYM_W		96
// RSS-14 height
#define SYM_H 33
// RSS-14 truncated height
#define TRNC_H 13
// RSS-14S row heights
#define ROWS1_H	5
#define ROWS2_H	7


#define L_PADR 5 // RSS-14 left offset
#define R_PADR 7 // RSS-14s right offset

extern int errFlag;
extern int line1;
extern uint8_t ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

static bool RSS14enc(uint8_t str[], uint8_t pattern[], int ccFlag);
static struct sPrints *separator14S(struct sParams *params, struct sPrints *prints);


void RSS14(struct sParams *params) {

struct sPrints prints;
struct sPrints *prntCnv;

uint8_t linPattern[ELMNTS];

char primaryStr[14+1];
char tempStr[28+1];

int i;
int rows, ccFlag;
char *ccStr;
int symHt;

	if (params->sym == sRSS14) {
		symHt = SYM_H;
	}
	else {
		symHt = TRNC_H;
	}
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

	if (RSS14enc((uint8_t*)primaryStr, linPattern, ccFlag)) {
		if (errFlag) {
			printf("\nRSS14 encoding error occurred.");
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
	line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = ELMNTS;
	prints.pattern = linPattern;
	prints.guards = true;
	prints.height = params->pixMult*symHt;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if ((rows = CC4enc((uint8_t*)ccStr, ccPattern)) > 0) {
			if (errFlag) {
				printf("\nComposite encoding error occurred.");
				return;
			}
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
		}
		if (params->bmp) {
			// note: BMP is bottom to top inverted
			bmpHeader(params->pixMult*CCB4_WIDTH,
					params->pixMult*(rows*2+symHt) + params->sepHt, params->outfp);

			// RSS-14
			prints.leftPad = L_PADR;
			printElmnts(params, &prints);

			// CC separator
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// Composite Component
			prints.elmCnt = CCB4_ELMNTS;
			prints.guards = false;
			prints.height = params->pixMult*2;
			prints.leftPad = 0;
			for (i = rows-1; i >= 0; i--) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}
		}
		else {
			tifHeader(params->pixMult*CCB4_WIDTH,
					params->pixMult*(rows*2+symHt) + params->sepHt, params->outfp);

			// Composite Component
			prints.elmCnt = CCB4_ELMNTS;
			prints.guards = false;
			prints.height = params->pixMult*2;
			for (i = 0; i < rows; i++) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}

			prints.elmCnt = ELMNTS;
			prints.pattern = linPattern;
			prints.guards = true;
			prints.height = params->pixMult*symHt;
			prints.leftPad = L_PADR;

			// CC separator
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS-14
			printElmnts(params, &prints);
		}
	}
	else { // primary only
		if (params->bmp) {
			bmpHeader(params->pixMult*SYM_W, params->pixMult*symHt, params->outfp);
		}
		else {
			tifHeader(params->pixMult*SYM_W, params->pixMult*symHt, params->outfp);
		}

		// RSS-14
		printElmnts(params, &prints);
	}
	return;
}

void RSS14S(struct sParams *params) {

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

	if (RSS14enc((uint8_t*)primaryStr, linPattern, ccFlag)) {
		if (errFlag) {
			printf("\nRSS14 encoding error occurred.");
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
	line1 = true; // so first line is not Y undercut
	// init most common RSS14S row prints values
	prints.elmCnt = ELMNTS/2;
	prints.guards = true;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if ((rows = CC2enc((uint8_t*)ccStr, ccPattern)) > 0) {
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
					for (j = 0; j < CCB2_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
					printf("\n");
				}
			}
#endif
		}

		if (params->bmp) {
			// note: BMP is bottom to top inverted
			bmpHeader(params->pixMult*(CCB2_WIDTH),
				params->pixMult*(rows*2+ROWS1_H+ROWS2_H) + 2*params->sepHt, params->outfp);

			// RSS14S lower row
			prints.height = params->pixMult*ROWS2_H;
			prints.pattern = &linPattern[ELMNTS/2];
			prints.rightPad = R_PADR;
			prints.whtFirst = false;
			printElmnts(params, &prints);

			// RSS14S separator pattern
			prints.pattern = linPattern;
			prntCnv = separator14S(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14S upper row
			prints.whtFirst = true;
			prints.height = params->pixMult*ROWS1_H;
			printElmnts(params, &prints);

			// CC separator
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// Composite Component
			prints.elmCnt = CCB2_ELMNTS;
			prints.guards = false;
			prints.height = params->pixMult*2;
			prints.rightPad = 0;
			for (i = rows-1; i >= 0; i--) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}
		}
		else {
			tifHeader(params->pixMult*(CCB2_WIDTH),
					params->pixMult*(rows*2+ROWS1_H+ROWS2_H) + 2*params->sepHt, params->outfp);

			// Composite Component
			prints.elmCnt = CCB2_ELMNTS;
			prints.guards = false;
			prints.height = params->pixMult*2;
			for (i = 0; i < rows; i++) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}

			// CC separator
			prints.elmCnt = ELMNTS/2;
			prints.pattern = linPattern;
			prints.rightPad = R_PADR;
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14S upper row
			prints.guards = true;
			prints.height = params->pixMult*ROWS1_H;
			printElmnts(params, &prints);

			// RSS14S separator pattern
			prntCnv = separator14S(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14S lower row
			prints.height = params->pixMult*ROWS2_H;
			prints.pattern = &linPattern[ELMNTS/2];
			prints.whtFirst = false;
			printElmnts(params, &prints);
		}
	}
	else { // primary only
		if (params->bmp) {
			bmpHeader(params->pixMult*(SYM_W/2+2),
				params->pixMult*(ROWS1_H+ROWS2_H) + params->sepHt, params->outfp);


			// RSS14S lower row
			prints.height = params->pixMult*ROWS2_H;
			prints.pattern = &linPattern[ELMNTS/2];
			prints.whtFirst = false;
			printElmnts(params, &prints);

			// RSS14S separator pattern
			prints.pattern = linPattern;
			prntCnv = separator14S(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14S upper row
			prints.whtFirst = true;
			prints.height = params->pixMult*ROWS1_H;
			printElmnts(params, &prints);
		}
		else {
			tifHeader(params->pixMult*(SYM_W/2+2),
					params->pixMult*(ROWS1_H+ROWS2_H) + params->sepHt, params->outfp);

			// RSS14S upper row
			prints.pattern = linPattern;
			prints.height = params->pixMult*ROWS1_H;
			printElmnts(params, &prints);

			// RSS14S separator pattern
			prntCnv = separator14S(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14S lower row
			prints.pattern = &linPattern[ELMNTS/2];
			prints.height = params->pixMult*ROWS2_H;
			prints.whtFirst = false;
			printElmnts(params, &prints);
		}
	}
	return;
}

void RSS14SO(struct sParams *params) {

struct sPrints prints;
struct sPrints chexPrnts;
struct sPrints *prntCnv;

uint8_t linPattern[ELMNTS];
uint8_t chexPattern[SYM_W/2+2];

char primaryStr[14+1];
char tempStr[28+1];

int i;
int rows, ccFlag;
char *ccStr;

	for (i = 0; i < SYM_W/2+2; i++) chexPattern[i] = 1; // chex = all 1X elements
	chexPattern[0] = 5; // wide space on left
	chexPattern[SYM_W/2+1-7] = 4; // wide space on right
	chexPrnts.elmCnt = SYM_W/2+2-7;
	chexPrnts.pattern = &chexPattern[0];
	chexPrnts.guards = false;
	chexPrnts.height = params->sepHt;
	chexPrnts.whtFirst = true;
	chexPrnts.leftPad = 0;
	chexPrnts.rightPad = 0; // assume not a composite for now
	chexPrnts.reverse = false;

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

	if (RSS14enc((uint8_t*)primaryStr, linPattern, ccFlag)) {
		if (errFlag) {
			printf("\nRSS14 encoding error occurred.");
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
	line1 = true; // so first line is not Y undercut
	// init most common RSS14SO row prints values
	prints.elmCnt = ELMNTS/2;
	prints.guards = true;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		chexPrnts.rightPad = R_PADR; // pad for composite
		if ((rows = CC2enc((uint8_t*)ccStr, ccPattern)) > 0) {
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
					for (j = 0; j < CCB2_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
					printf("\n");
				}
			}
#endif
		}

		if (params->bmp) {
			// note: BMP is bottom to top inverted
			bmpHeader(params->pixMult*(CCB2_WIDTH),
				params->pixMult*(rows*2+SYM_H*2) + 4*params->sepHt, params->outfp);

			// RSS14SO lower row
			prints.height = params->pixMult*SYM_H;
			prints.pattern = &linPattern[ELMNTS/2];
			prints.rightPad = R_PADR;
			prints.whtFirst = false;
			printElmnts(params, &prints);

			// RSS14SO lower row separator pattern
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// chex pattern
			printElmnts(params, &chexPrnts);

			// RSS14SO upper row separator pattern
			prints.pattern = linPattern;
			prints.whtFirst = true;
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14SO upper row
			printElmnts(params, &prints);

			// CC separator
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// Composite Component
			prints.elmCnt = CCB2_ELMNTS;
			prints.guards = false;
			prints.height = params->pixMult*2;
			prints.rightPad = 0;
			for (i = rows-1; i >= 0; i--) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}
		}
		else {
			tifHeader(params->pixMult*(CCB2_WIDTH),
				params->pixMult*(rows*2+SYM_H*2) + 4*params->sepHt, params->outfp);

			// Composite Component
			prints.elmCnt = CCB2_ELMNTS;
			prints.guards = false;
			prints.height = params->pixMult*2;
			for (i = 0; i < rows; i++) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}

			// CC separator
			prints.elmCnt = ELMNTS/2;
			prints.pattern = linPattern;
			prints.rightPad = R_PADR;
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14SO upper row
			prints.guards = true;
			prints.height = params->pixMult*SYM_H;
			printElmnts(params, &prints);

			// RSS14SO upper row separator pattern
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// chex pattern
			printElmnts(params, &chexPrnts);

			// RSS14SO lower row separator pattern
			prints.pattern = &linPattern[ELMNTS/2];
			prints.whtFirst = false;
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14SO lower row
			prints.height = params->pixMult*SYM_H;
			printElmnts(params, &prints);
		}
	}
	else { // primary only
		if (params->bmp) {
			bmpHeader(params->pixMult*(SYM_W/2+2),
				params->pixMult*(SYM_H*2) + 3*params->sepHt, params->outfp);

			// RSS14SO lower row
			prints.height = params->pixMult*SYM_H;
			prints.pattern = &linPattern[ELMNTS/2];
			prints.whtFirst = false;
			printElmnts(params, &prints);

			// RSS14SO lower row separator pattern
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// chex pattern
			printElmnts(params, &chexPrnts);

			// RSS14SO upper row separator pattern
			prints.pattern = linPattern;
			prints.whtFirst = true;
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14SO upper row
			prints.height = params->pixMult*SYM_H;
			printElmnts(params, &prints);
		}
		else {
			tifHeader(params->pixMult*(SYM_W/2+2),
				params->pixMult*(SYM_H*2) + 3*params->sepHt, params->outfp);

			// RSS14SO upper row
			prints.pattern = linPattern;
			prints.height = params->pixMult*SYM_H;
			printElmnts(params, &prints);

			// RSS14SO upper row separator pattern
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// chex pattern
			printElmnts(params, &chexPrnts);

			// RSS14SO lower row separator pattern
			prints.pattern = &linPattern[ELMNTS/2];
			prints.whtFirst = false;
			prntCnv = cnvSeparator(params, &prints);
			printElmnts(params, prntCnv);

			// RSS14SO lower row
			prints.height = params->pixMult*SYM_H;
			printElmnts(params, &prints);
		}
	}
	return;
}


static struct sPrints prntSep;
static uint8_t sepPattern[SYM_W/2+2];

// RSS14 Stacked row separator pattern routine
static struct sPrints *separator14S(struct sParams *params, struct sPrints *prints) {
int i, j, k, lNdx, rNdx, sNdx, lWidth, rWidth, matchWidth;

	prntSep.leftPad = prints->leftPad;
	prntSep.rightPad = prints->rightPad;
	prntSep.reverse = prints->reverse;
	prntSep.pattern = sepPattern;
	prntSep.height = params->sepHt;
	prntSep.whtFirst = true;
	prntSep.guards = false;

	sepPattern[0] = sepPattern[1] = 1; // start with old SB guard in separator
	lNdx = 0; // left (top) element index
	rNdx = ELMNTS/2; // right (bottom) element index
	sNdx = 2; // separator element index
	lWidth = rWidth = matchWidth = 0;
	for (i = 0; i < SYM_W/2 - 2; i++, lWidth--, rWidth--) {
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
	prntSep.elmCnt = j+1;
	return(&prntSep);
}


#define K	4
// call with str = 13-digit primary, no check digit
static bool RSS14enc(uint8_t string[], uint8_t bars[], int ccFlag) {

	#define	PARITYCHRSIZE	9
	#define PARITY_MOD 79

	// left char multiplier
	#define LEFT_MUL 4537077.

	// outside semi-char multipliers
	#define	SEMI_MUL	1597

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
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
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
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
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
