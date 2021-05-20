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

#include "gs1encoders.h"
#include "enc-private.h"
#include "cc.h"
#include "driver.h"
#include "ucc128.h"

#define ISNUM(A) ((A<072)&&(A>057)) /* true if A is numeric ASCII */


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

	while ((data[di] != 0) && (si < UCC128_SYMMAX - 2 - (link > 0 ? 1:0))) {

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


void gs1_U128A(gs1_encoder *ctx) {

	struct sPrints prints;

	uint8_t linPattern[(UCC128_SYMMAX*6)+3];

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	char dataStr[GS1_ENCODERS_MAX_DATA+1];

	int i;
	int rows, ccFlag, symChars, symWidth, ccLpad, ccRpad;
	char primaryStr[120+1];
	char *ccStr;

	strcpy(dataStr, ctx->dataStr);
	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(dataStr) > 48) {
		strcpy(ctx->errMsg, "primary data exceeds 48 characters");
		ctx->errFlag = true;
		return;
	}

	// insert leading FNC1 if not already there
	if (dataStr[0] != '#') {
		strcpy(primaryStr, "#");
	}
	else {
		primaryStr[0] = '\0';
	}
	strcat(primaryStr, dataStr);

	symChars = enc128((uint8_t*)primaryStr, linPattern, (ccFlag) ? 1 : 0);

#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < (symChars*6+3); i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif

	ctx->line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = symChars*6+3;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = ctx->pixMult * ctx->linHeight;
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

		if (symChars < 9) {
			strcpy(ctx->errMsg, "linear component too short");
			ctx->errFlag = true;
			return;
		}

		symWidth = symChars*11+22;
		ccRpad = 10+2 + ((symChars-9)/2)*11;
		ccLpad = symWidth - (CCB4_WIDTH + ccRpad);

		gs1_driverInit(ctx, ctx->pixMult*symWidth,
				ctx->pixMult*(rows*2+ctx->linHeight) + ctx->sepHt);

		// CC-C
		prints.elmCnt = CCB4_ELMNTS;
		prints.height = ctx->pixMult*2;
		prints.leftPad = ccLpad;
		prints.rightPad = ccRpad;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator pattern
		prints.elmCnt = symChars*6+1;
		prints.pattern = &linPattern[1];
		prints.height = ctx->sepHt;
		prints.leftPad = 10;
		prints.rightPad = 10;
		gs1_driverAddRow(ctx, &prints);

		// UCC-128
		prints.elmCnt = symChars*6+3;
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*ctx->linHeight;
		prints.leftPad = 0;
		prints.rightPad = 0;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*(symChars*11+22), ctx->pixMult*ctx->linHeight);

		// UCC-128
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	return;
}


void gs1_U128C(gs1_encoder *ctx) {

	struct sPrints prints;
	uint8_t *patCCC = ctx->ucc128_patCCC;

	uint8_t linPattern[(UCC128_SYMMAX*6)+3];

	char dataStr[GS1_ENCODERS_MAX_DATA+1];

	int i;
	int ccFlag, symChars, symWidth, ccRpad;
	char primaryStr[120+1];
	char *ccStr;

	strcpy(dataStr, ctx->dataStr);
	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
	}

	if (strlen(dataStr) > 48) {
		strcpy(ctx->errMsg, "primary data exceeds 48 characters");
		ctx->errFlag = true;
		return;
	}

	// insert leading FNC1 if not already there
	if (dataStr[0] != '#') {
		strcpy(primaryStr, "#");
	}
	else {
		primaryStr[0] = '\0';
	}
	strcat(primaryStr, dataStr);

	symChars = enc128((uint8_t*)primaryStr, linPattern, (ccFlag) ? 2 : 0); // 2 for CCC

#if PRNT
	printf("\n%s", primaryStr);
	printf("\n");
	for (i = 0; i < (symChars*6+3); i++) {
		printf("%d", linPattern[i]);
	}
	printf("\n");
#endif

	ctx->colCnt = ((symChars*11 + 22 - UCC128_L_PAD - 5)/17) -4;
	if (ctx->colCnt < 1) {
		strcpy(ctx->errMsg, "UCC-128 too small");
		ctx->errFlag = true;
		return;
	}
	ctx->line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = symChars*6+3;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = ctx->pixMult*ctx->linHeight;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	if (ccFlag) {
		if (!gs1_CCCenc(ctx, (uint8_t*)ccStr, patCCC) || ctx->errFlag) return;
#if PRNT
		{
			int j;
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < ctx->rowCnt; i++) {
				for (j = 0; j < (ctx->colCnt+4)*8+3; j++) {
					printf("%d", patCCC[i*((ctx->colCnt+4)*8+3) + j]);
				}
				printf("\n");
			}
		}
#endif

		symWidth = symChars*11+22;
		ccRpad = symWidth - UCC128_L_PAD - ((ctx->colCnt+4)*17+5);

		gs1_driverInit(ctx, ctx->pixMult*symWidth,
				ctx->pixMult*(ctx->rowCnt*3+ctx->linHeight) + ctx->sepHt);

		// CC-C
		prints.elmCnt = (ctx->colCnt+4)*8+3;
		prints.height = ctx->pixMult*3;
		prints.leftPad = UCC128_L_PAD;
		prints.rightPad = ccRpad;
		for (i = 0; i < ctx->rowCnt; i++) {
			prints.pattern = &patCCC[i*((ctx->colCnt+4)*8+3)];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator pattern
		prints.elmCnt = symChars*6+1;
		prints.pattern = &linPattern[1];
		prints.height = ctx->sepHt;
		prints.leftPad = 10;
		prints.rightPad = 10;
		gs1_driverAddRow(ctx, &prints);

		// UCC-128
		prints.elmCnt = symChars*6+3;
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*ctx->linHeight;
		prints.leftPad = 0;
		prints.rightPad = 0;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*(symChars*11+22), ctx->pixMult*ctx->linHeight);

		// UCC-128
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	return;
}


#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_ucc_UCC128A_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
"          XX X  X    XXXX X XXX X  XXXX X  X XX  X    X XXXX  X  X  XXXX X  X    XX X  XX    X X  X  XX X    XX  XX XX  XX   XXX X XX          ",
"vvv",
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUCC128_CCA, "testing", expect));

	expect = (char*[]){
"                    XX XX XXX XXX  XX XXX XXXX X   X X     X    X  XXX X  XX  XX    XX  XX XXXX  XXXX  X X  XX XX   X X                        ",
"                    XX XX XXX XXX  XX XXX XXXX X   X X     X    X  XXX X  XX  XX    XX  XX XXXX  XXXX  X X  XX XX   X X                        ",
"                    XX XX XX  XXXX XX  XX   X  XXX X XXXX   XXX X  XX  X  X XXXXX  X   XXX XXXXX X   XX XXX XX  X   X X                        ",
"                    XX XX XX  XXXX XX  XX   X  XXX X XXXX   XXX X  XX  X  X XXXXX  X   XXX XXXXX X   XX XXX XX  X   X X                        ",
"                    XX XX X   X  XX  X XXX     XXX   X XXXXX  X X  XX  XX X XX X    XXX    XX XXXXXX XX X   XXX X   X X                        ",
"                    XX XX X   X  XX  X XXX     XXX   X XXXXX  X X  XX  XX X XX X    XXX    XX XXXXXX XX X   XXX X   X X                        ",
"            X XX   XX    X X   X X  XX   XX  X  XXX  X   X  X   X X  XX   XX  X  XXX  X   X  X   X   X X    X    X XX XX  XXX   X X            ",
"          XX X  XXX  XXXX X XXX X XX  XXX  XX XX   XX XXX XX XXX X XX  XXX  XX XX   XX XXX XX XXX XXX X XXXX XXXX X  X  XX   XXX X XX          ",
"vvv",
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUCC128_CCA, "123123123123|99123123", expect));

	gs1_encoder_free(ctx);

}


void test_ucc_UCC128C_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
"   XXXXXXXX X X X   XXXX X X XXXX    XX X XXX XXXX    XXX XXXXX XXX X  X     X    X   X XXXX X XX  XXXXX XXX X  X   XXX   X   X   XX    XX XXXXX X X  XXXXX XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXXX X X XXXX    XX X XXX XXXX    XXX XXXXX XXX X  X     X    X   X XXXX X XX  XXXXX XXX X  X   XXX   X   X   XX    XX XXXXX X X  XXXXX XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXXX X X XXXX    XX X XXX XXXX    XXX XXXXX XXX X  X     X    X   X XXXX X XX  XXXXX XXX X  X   XXX   X   X   XX    XX XXXXX X X  XXXXX XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXXXXX X X   XXX X    X  XXXX X   XX  XXXX X XXXXX XXXX X   X X     XXXXX   XX  X X  XXX  X XXX   XX  X  X  XXXXX XX   XXXXXX X X XXX   XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXXXXX X X   XXX X    X  XXXX X   XX  XXXX X XXXXX XXXX X   X X     XXXXX   XX  X X  XXX  X XXX   XX  X  X  XXXXX XX   XXXXXX X X XXX   XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXXXXX X X   XXX X    X  XXXX X   XX  XXXX X XXXXX XXXX X   X X     XXXXX   XX  X X  XXX  X XXX   XX  X  X  XXXXX XX   XXXXXX X X XXX   XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   X X X   XXXX     XX  XXXXX    X X XX X   X XXXXX   XXX XXXX X XXX   XX     XX X XXXX X XXXX  X      X XX    X XXXXXX X XXX X X   XXXXXX XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   X X X   XXXX     XX  XXXXX    X X XX X   X XXXXX   XXX XXXX X XXX   XX     XX X XXXX X XXXX  X      X XX    X XXXXXX X XXX X X   XXXXXX XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   X X X   XXXX     XX  XXXXX    X X XX X   X XXXXX   XXX XXXX X XXX   XX     XX X XXXX X XXXX  X      X XX    X XXXXXX X XXX X X   XXXXXX XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   X X XXXX  XXXX   XXXX X    X XXXX X X XXXXX XXXXX  XX X X    XX     XX X  XXX   XXXX X XXXX  XXXX X   XX   XXX XX X    XXX X  X XXX     XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   X X XXXX  XXXX   XXXX X    X XXXX X X XXXXX XXXXX  XX X X    XX     XX X  XXX   XXXX X XXXX  XXXX X   XX   XXX XX X    XXX X  X XXX     XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   X X XXXX  XXXX   XXXX X    X XXXX X X XXXXX XXXXX  XX X X    XX     XX X  XXX   XXXX X XXXX  XXXX X   XX   XXX XX X    XXX X  X XXX     XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXX X XXX    XX  XXXXX X X   XX   X  X    XXXX   X XXX      X XX  X XXXX  X XX   XX  XXXX  X XX XX    XXXX XX   XX X   XXX X XXX  XX    XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXX X XXX    XX  XXXXX X X   XX   X  X    XXXX   X XXX      X XX  X XXXX  X XX   XX  XXXX  X XX XX    XXXX XX   XX X   XXX X XXX  XX    XXXXXXX X   X X  X             ",
"   XXXXXXXX X X X   XXX X XXX    XX  XXXXX X X   XX   X  X    XXXX   X XXX      X XX  X XXXX  X XX   XX  XXXX  X XX XX    XXXX XX   XX X   XXX X XXX  XX    XXXXXXX X   X X  X             ",
"            X XX   XX    X X   X  X  XX  XX XX XX  XXX  XX  X  XX   X  X   X X   X  XXX XXXX X  XX  X  X    X  XX  X  XX   X  X   X  XXX X XXX X    X   X   X    X X  XXX   X X            ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
"          XX X  XXX  XXXX X XXX XX XX  XX  X  X  XX   XX  XX XX  XXX XX XXX X XXX XX   X    X XX  XX XX XXXX XX  XX XX  XXX XX XXX XX   X X   X XXXX XXX XXX XXXX X XX   XXX X XX          ",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUCC128_CCC, "00030123456789012340|02130123456789093724#101234567ABCDEFG", expect));

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
