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

#include "enc-private.h"
#include "cc.h"
#include "driver.h"
#include "ucc128.h"

#define ISNUM(A) ((A<072)&&(A>057)) /* true if A is numeric ASCII */


// TODO move into context
// globals used by CC-C:
int colCnt; // after set in main, may be decreased by getUnusedBitCnt
int rowCnt; // determined by getUnusedBitCnt
int eccCnt; // determined by getUnusedBitCnt


extern int errFlag;
extern int line1;
extern uint8_t ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];


/*
 * tbl128 converts an array of Code 128 symbol values to an array of
 * bar and space widths which represent that symbol.
 *
 * Calling Parameters:
 *
 * symchr  int[]    array of code 128 symbol values, -1 terminator
 * bars    int[]    array to be filled with bar & space widths
 *
 * return:    none
 *
 */
static void tbl128(int symchr[], uint8_t bars[])
{

	/* octal of 1st 5 elements in symbol char */
	static const int sym128[107] ={
		021222,022212,022222,012122,012132,
		013122,012221,012231,013221,022121,
		022131,023121,011223,012213,012223,
		011322,012312,012322,022321,022113,
		022123,021321,022311,031213,031122,
		032112,032122,031221,032211,032221,
		021212,021232,023212,011132,013112,
		013132,011231,013211,013231,021131,
		023111,023131,011213,011233,013213,
		011312,011332,013312,031312,021133,
		023113,021311,021331,021313,031112,
		031132,033112,031211,031231,033211,
		031411,022141,043111,011122,011142,
		012112,012142,014112,014122,011221,
		011241,012211,012241,014211,014221,
		024121,022111,041311,024111,013411,
		011124,012114,012124,011421,012411,
		012421,041121,042111,042121,021214,
		021412,041212,011114,011134,013114,
		011411,011431,041111,041131,011314,
		011413,031114,041113,021141,021121,
		021123,023311
	};
	int si, bi, pattern;
	int val;
	uint8_t i;

	/* look up symchr[]'s and copy widths into bars[] */
	si = bi = 0;
	bars[bi++] = 10; /* leading qz */
	while ((val = symchr[si++]) != -1) {
		pattern = sym128[val];

		/* shift out octal digits in pattern */
		i = bars[bi + 4] = (uint8_t)pattern % 8;
		pattern /= 8;
		bars[bi + 3] = (uint8_t)pattern % 8;
		i = (uint8_t)(i + pattern % 8);
		pattern /= 8;
		bars[bi + 2] = (uint8_t)pattern % 8;
		i = (uint8_t)(i + pattern % 8);
		pattern /= 8;
		bars[bi + 1] = (uint8_t)pattern % 8;
		i = (uint8_t)(i + pattern % 8);
		bars[bi] = (uint8_t)pattern / 8;
		i = (uint8_t)(i + pattern / 8);

		/* derive last space to total 11 */
		bars[bi + 5] = (uint8_t)(11 - i);
		bi += 6;
	}
	bars[bi++] = 2;  /* trailing guard bar */
	bars[bi++] = 10; /* trailing qz */
	bars[bi] = 0;    /* array terminator */
	return;
}


/*
 * cda128 converts data into a symbol character in code set A.
 *
 *
 * Calling Parameters:
 *
 * data    uchar[]  string of ASCII data to be encoded with 0200 for
 *				 NUL and 0201-0204 for FNC1-FNC4
 * di      *char    next index into data
 * symchr  int[]    array of symbol character values to be filled.
 * si      *int     next index into symchr
 * code    *int     starting code: A,B,C = 0,1,2
 *
 */
static void cda128(uint8_t data[], int *di, int symchr[], int *si, int *code)
{
	int   c, i;

	c = data[*di];

	/* look for numeric field next */
	for (i = *di; ISNUM(data[i]); i++);
	if (((i - *di) > 3) && (((i - *di) & 1) == 0)) {
		/* 4 or more even digits, change to code C */
		*code = 2;
		symchr[(*si)++] = 99;
	}
	else if ((c > 0137) && (c < 0200)) {
		/* lower case, change to code B */
		*code = 1;
		symchr[(*si)++] = 100;
	}
	else { /* process char in code A */
		if (c < 040) c += 64;       /*cntl char*/
		else if (c < 0140) c -= 32; /*alphanumeric*/
		else {
			if (c == 0200) c = 64;  /*null*/
			if (c == 0201) c = 102; /*FNC1*/
			if (c == 0202) c = 97;  /*FNC2*/
			if (c == 0203) c = 96;  /*FNC3*/
			if (c == 0204) c = 101; /*FNC4*/
		}
		symchr[(*si)++] = c;
		++(*di);
	}
	return;
}


/*
 * cdb128 converts data into a symbol character in code set B.
 *
 * Calling Parameters:
 *
 * data    uchar[]  string of ASCII data to be encoded with 0200 for
 *				 NUL and 0201-0204 for FNC1-FNC4
 * di      *char    next index into data
 * symchr  int[]    array of symbol character values to be filled.
 * si      *int     next index into symchr
 * code    *int     starting code: A,B,C = 0,1,2
 *
 */
static void cdb128(uint8_t data[], int *di, int symchr[], int *si, int *code)
{
	int   c, i;

	c = data[*di];

	/* look for numeric field next */
	for (i = *di; ISNUM(data[i]); i++);
	if (((i - *di) > 3) && (((i - *di) & 1) == 0)) {
		/* 4 or more even digits, change to code C */
		*code = 2;
		symchr[(*si)++] = 99;
	}
	else if ((c < 040) || (c == 0200)) {
		/* control char, change to code A */
		*code = 0;
		symchr[(*si)++] = 101;
	}
	else {   /* process char in code B */
		if (c < 0200) c -= 32; /*alphanumerics*/
		else {
			if (c == 0201) c = 102; /*FNC1*/
			if (c == 0202) c = 97;  /*FNC2*/
			if (c == 0203) c = 96;  /*FNC3*/
			if (c == 0204) c = 100; /*FNC4*/
		}
		symchr[(*si)++] = c;
		++(*di);
	}
	return;
}

/*
 * cdc128 converts data into a symbol character in code set C.
 *
 *
 * Calling Parameters:
 *
 * data    uchar[]  string of ASCII data to be encoded with 0200 for
 *				 NUL and 0201-0204 for FNC1-FNC4
 * di      *char    next index into data
 * symchr  int[]    array of symbol character values to be filled.
 * si      *int     next index into symchr
 * code    *int     starting code: A,B,C = 0,1,2
 *
 */
static void cdc128(uint8_t data[], int *di, int symchr[], int *si, int *code)
{
	int c, i;

	c = data[*di];

	/* check if next 2 chars numeric */
	if (ISNUM(c) && ISNUM(data[*di+1])) {
		/* 2 numerics, pack them into 1 */
		symchr[(*si)++] = ((c & 0xF) * 10) + (data[++(*di)] & 0xF);
		++(*di);
	}
	/* else is next a FNC1? */
	else if (c == 0201) {
		symchr[(*si)++] = 102; /* FNC1 */
		(*di)++;
	}
	else {   /* decide between A and B */
		for (i = *di;  /* search for next cntl or lower case */
			((data[i] >= 040) &&
			 ((data[i] <= 0137) || (data[i] >= 0201)));
						i++);

		if ((data[i] < 040) || (data[i] == 0200)) {
			/* control char or end of data, change to code A */
			*code = 0;
			symchr[(*si)++] = 101;
		}
		else {  /* lower case alpha, change to code B */
			*code = 1;
			symchr[(*si)++] = 100;
		}
	}
	return;
}


/*
 * enc128 converts the data string into a Code 128 symbol
 * represented in an array of bar and space widths.
 *
 * Subroutines used:
 *
 * cda128    converts data to a symbol character in code set A.
 * cdb128    converts data to a symbol character in code set B.
 * cdc128    converts data to a symbol character in code set C.
 * tbl128    performs translation from symbol characters to
 *			bar/space widths.
 *
 * Calling Parameters:
 *
 * data    uchar[]  string of ASCII data to be encoded with 0200 for
 *				 NUL and 0201-0204 for FNC1-FNC4
 * bars    int[]    array of bar/space widths to be filled, 0 term.
 *
 * Function Return:    number of symbol characters
 *
 */
static int enc128(uint8_t data[], uint8_t bars[], int link)
{
	/* convert ASCII data[] into symchr[] values */

	static const int linkChar[3][2] = { { 100,99 }, { 99,101 }, { 101,100 } };
	int si, di, i, code;
	int symchr[UCC128_SYMMAX+1];
	long ckchr;

	for (i = 0; i < (int)strlen((char*)data); i++) {
		if (data[i] == '#') {
			data[i] = 0201; // convert FNC1 to 201 octal for enc128
		}
	}

	/* determine start character A, B or C */

	di = 0;
	if (data[0] == 0201) di++;  /* skip over leading FNC1 */
	i = di;
	while (ISNUM(data[i])) { i++; } /* look for leading numerics */
	if ((i - di) >= 4) code = 2;  /* 4 or more, code C */
	else if (((i - di) == 2) && data[i] == '\0') code = 2; // NN: code C
	else {   /* decide between A and B */
		for (i = di;  /* search for next cntl or lower case */
			((data[i] >= 040) &&
		 		((data[i] <= 0137) || (data[i] >= 0201)));
					i++);

		if ((data[i] < 040) || (data[i] == 0200))
				/* control char or end of data, change to code A */
				code = 0;
		else  /* lower case alpha, change to code B */
				code = 1;
	}

	symchr[0] = 103 + code;      /*start char A, B or C*/

	di = 0;
	si = 1;

	/* convert ASCII data to symbol characters */
	/* loop until data or symchr array expires */

	while ((data[di] != 0) && (si < UCC128_SYMMAX-2)) {

		switch (code) {

			case 0:   /* code A */
				cda128(data, &di, symchr, &si, &code);
				break;

			case 1:   /* code B */
				cdb128(data, &di, symchr, &si, &code);
				break;

			case 2:  /* code C */
				cdc128(data, &di, symchr, &si, &code);
				break;
		}
	}

	if (link > 0) {
		// insert trailing Code Set Char flag
		symchr[si++] = linkChar[code][link-1];
	}

	/* calculate check char */

	i = 0;
	ckchr = symchr[0];
	while (++i < si) ckchr += (symchr[i] * i);

	symchr[si++] = (int)(ckchr % 103);  /* store check char */
	symchr[si++] = 106;          /* store stop char */
	symchr[si] = -1;             /* -1 terminator */

	/* translate symbol characters to bars and spaces */

	tbl128(symchr, bars);

	return(si);
}


void U128A(gs1_encoder *params) {

	struct sPrints prints;

	uint8_t linPattern[(UCC128_SYMMAX*6)+3];

	int i;
	int rows, ccFlag, symChars, symWidth, ccLpad, ccRpad;
	char primaryStr[120+1];
	char *ccStr;

	ccStr = strchr(params->dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(params->dataStr) > 48) {
		errMsg = "primary data exceeds 48 characters";
		errFlag = true;
		return;
	}

	// insert leading FNC1 if not already there
	if (params->dataStr[0] != '#') {
		strcpy(primaryStr, "#");
	}
	else {
		primaryStr[0] = '\0';
	}
	strcat(primaryStr, params->dataStr);

	symChars = enc128((uint8_t*)primaryStr, linPattern, (ccFlag) ? 1 : 0);

#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < (symChars*6+3); i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif

	line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = symChars*6+3;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = params->pixMult*params->linHeight;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if (!((rows = CC4enc(params, (uint8_t*)ccStr, ccPattern)) > 0) || errFlag) return;
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

		if (symChars < 9) {
			errMsg = "linear component too short";
			errFlag = true;
			return;
		}

		symWidth = symChars*11+22;
		ccRpad = 10+2 + ((symChars-9)/2)*11;
		ccLpad = symWidth - (CCB4_WIDTH + ccRpad);

		if (params->bmp) {
			// note: BMP is bottom to top inverted
			bmpHeader(params->pixMult*symWidth,
					params->pixMult*(rows*2+params->linHeight) + params->sepHt,
						 params->outfp);

			// UCC-128
			printElmnts(params, &prints);

			// CC separator pattern
			prints.elmCnt = symChars*6+1;
			prints.pattern = &linPattern[1];
			prints.height = params->sepHt;
			prints.leftPad = 10;
			prints.rightPad = 10;
			printElmnts(params, &prints);

			// CC-C
			prints.elmCnt = CCB4_ELMNTS;
			prints.height = params->pixMult*2;
			prints.leftPad = ccLpad;
			prints.rightPad = ccRpad;
			for (i = rows-1; i >= 0; i--) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}
		}
		else {
			tifHeader(params->pixMult*symWidth,
					params->pixMult*(rows*2+params->linHeight) + params->sepHt,
						 params->outfp);

			// CC-C
			prints.elmCnt = CCB4_ELMNTS;
			prints.height = params->pixMult*2;
			prints.leftPad = ccLpad;
			prints.rightPad = ccRpad;
			for (i = 0; i < rows; i++) {
				prints.pattern = ccPattern[i];
				printElmnts(params, &prints);
			}

			// CC separator pattern
			prints.elmCnt = symChars*6+1;
			prints.pattern = &linPattern[1];
			prints.height = params->sepHt;
			prints.leftPad = 10;
			prints.rightPad = 10;
			printElmnts(params, &prints);

			// UCC-128
			prints.elmCnt = symChars*6+3;
			prints.pattern = linPattern;
			prints.height = params->pixMult*params->linHeight;
			prints.leftPad = 0;
			prints.rightPad = 0;
			printElmnts(params, &prints);
		}
	}
	else { // primary only
		if (params->bmp) {
			bmpHeader(params->pixMult*(symChars*11+22), params->pixMult*params->linHeight, params->outfp);
		}
		else {
			tifHeader(params->pixMult*(symChars*11+22), params->pixMult*params->linHeight, params->outfp);
		}

		// UCC-128
		printElmnts(params, &prints);
	}
	return;
}


void U128C(gs1_encoder *params) {

	struct sPrints prints;
	uint8_t *patCCC = params->ucc128_patCCC;

	uint8_t linPattern[(UCC128_SYMMAX*6)+3];

	int i;
	int ccFlag, symChars, symWidth, ccRpad;
	char primaryStr[120+1];
	char *ccStr;

	ccStr = strchr(params->dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(params->dataStr) > 48) {
		errMsg = "primary data exceeds 48 characters";
		errFlag = true;
		return;
	}

	// insert leading FNC1 if not already there
	if (params->dataStr[0] != '#') {
		strcpy(primaryStr, "#");
	}
	else {
		primaryStr[0] = '\0';
	}
	strcat(primaryStr, params->dataStr);

	symChars = enc128((uint8_t*)primaryStr, linPattern, (ccFlag) ? 2 : 0); // 2 for CCC

#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < (symChars*6+3); i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif

	colCnt = ((symChars*11 + 22 - UCC128_L_PAD - 5)/17) -4;
	if (colCnt < 1) {
		errMsg = "UCC-128 too small";
		errFlag = true;
		return;
	}
	line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = symChars*6+3;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = params->pixMult*params->linHeight;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if (!CCCenc(params, (uint8_t*)ccStr, patCCC) || errFlag) return;
#if PRNT
		{
			int j;
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < rowCnt; i++) {
				for (j = 0; j < (colCnt+4)*8+3; j++) {
					printf("%d", patCCC[i*((colCnt+4)*8+3) + j]);
				}
				printf("\n");
			}
		}
#endif

		symWidth = symChars*11+22;
		ccRpad = symWidth - UCC128_L_PAD - ((colCnt+4)*17+5);
		if (params->bmp) {
			// note: BMP is bottom to top inverted
			bmpHeader(params->pixMult*symWidth,
					params->pixMult*(rowCnt*3+params->linHeight) + params->sepHt,
						 params->outfp);

			// UCC-128
			printElmnts(params, &prints);

			// CC separator pattern
			prints.elmCnt = symChars*6+1;
			prints.pattern = &linPattern[1];
			prints.height = params->sepHt;
			prints.leftPad = 10;
			prints.rightPad = 10;
			printElmnts(params, &prints);

			// CC-C
			prints.elmCnt = (colCnt+4)*8+3;
			prints.height = params->pixMult*3;
			prints.leftPad = UCC128_L_PAD;
			prints.rightPad = ccRpad;
			for (i = rowCnt-1; i >= 0; i--) {
				prints.pattern = &patCCC[i*((colCnt+4)*8+3)];
				printElmnts(params, &prints);
			}
		}
		else {
			tifHeader(params->pixMult*symWidth,
					params->pixMult*(rowCnt*3+params->linHeight) + params->sepHt,
						 params->outfp);

			// CC-C
			prints.elmCnt = (colCnt+4)*8+3;
			prints.height = params->pixMult*3;
			prints.leftPad = UCC128_L_PAD;
			prints.rightPad = ccRpad;
			for (i = 0; i < rowCnt; i++) {
				prints.pattern = &patCCC[i*((colCnt+4)*8+3)];
				printElmnts(params, &prints);
			}

			// CC separator pattern
			prints.elmCnt = symChars*6+1;
			prints.pattern = &linPattern[1];
			prints.height = params->sepHt;
			prints.leftPad = 10;
			prints.rightPad = 10;
			printElmnts(params, &prints);

			// UCC-128
			prints.elmCnt = symChars*6+3;
			prints.pattern = linPattern;
			prints.height = params->pixMult*params->linHeight;
			prints.leftPad = 0;
			prints.rightPad = 0;
			printElmnts(params, &prints);
		}
	}
	else { // primary only
		if (params->bmp) {
			bmpHeader(params->pixMult*(symChars*11+22), params->pixMult*params->linHeight, params->outfp);
		}
		else {
			tifHeader(params->pixMult*(symChars*11+22), params->pixMult*params->linHeight, params->outfp);
		}

		// UCC-128
		printElmnts(params, &prints);
	}
	return;
}
