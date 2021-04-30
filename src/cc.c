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

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "enc.h"
#include "cc.h"

#define MAX_CCA2_SIZE 6		// index to 167 in CC2Sizes
#define MAX_CCA3_SIZE 4		// index to 167 in CC3Sizes
#define MAX_CCA4_SIZE 4		// index to 197 in CC4Sizes

#define MAX_CW 176		// ccb-4 max codewords
#define MAX_BYTES 148		// maximum byte mode capacity for ccb4

#define MAX_CCC_CW 863		// ccc max data codewords
#define MAX_CCC_ROWS 90		// ccc max rows
#define MAX_CCC_BYTES 1033	// maximum byte mode capacity for ccc

#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

struct encodeT {
	uint8_t *str;
	int iStr;
	uint8_t *bitField;
	int iBit;
	int mode;
	int typeAI;
	int diNum;
	int diAlpha;
};

enum {
	AIx,
	AIdummy, // not used
	AI21,
	AI8004
};

extern int errFlag;
extern int rowWidth;
extern int line1;
extern int linFlag; // tells pack whether linear, cc-a/b or cc-c is being encoded
extern uint8_t ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

// CC-C external variables
extern int colCnt; // after set in main, may be decreased by getUnusedBitCnt
extern int rowCnt; // determined by getUnusedBitCnt
extern int eccCnt; // determined by getUnusedBitCnt

static void encCCA2(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void encCCB2(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void encCCA3(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void encCCB3(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void encCCA4(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void encCCB4(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void encCCC(int byteCnt, uint8_t bitField[], uint16_t codeWords[],
		uint8_t patCCC[]);
static void imgCCA2(int size, uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void imgCCB2(int size, uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void imgCCA3(int size, uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void imgCCB3(int size, uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void imgCCA4(int size, uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void imgCCB4(int size, uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
static void imgCCC(uint16_t codeWords[], uint8_t patCCC[]);

static int procNUM(struct encodeT *encode);
static int procALNU(struct encodeT *encode);
static int procISO(struct encodeT *encode);
static int procALPH(struct encodeT *encode);
static int doMethods(struct encodeT *encode);
static int insertPad(struct encodeT *encode);
static int getUnusedBitCnt(int iBit, int *size);
static int testAI90(struct encodeT *encode);
static void procAI90(struct encodeT *encode);
static void encodeAI90(struct encodeT *encode);
static void nextAI(struct encodeT *encode);
static int encode928(uint8_t bitString[], uint16_t codeWords[], int bitLng);
static void encode900(uint8_t byteArr[], uint16_t codeWords[], int byteLng);
static int getBit(uint8_t bitStr[], int bitPos);

static void genECC(int dsize, int csize, uint16_t sym[]);
static void genPoly(int eccSize);
static int gfMul(int p1, int p2);

static int doLinMethods(uint8_t str[], int *iStr, uint8_t bitField[], int *iBit);
static uint16_t yymmdd(uint8_t str[]);
static void cnv13 (uint8_t str[], int *iStr, uint8_t bitField[], int *iBit);
static void cnv12 (uint8_t str[], int *iStr, uint8_t bitField[], int *iBit);

static int *CCSizes; // will point to CCxSizes

static int CC2Sizes[] = {	59,78,88,108,118,138,167,	// cca sizes
				208,256,296,336,		// ccb sizes
				0 };

int CC2enc(uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS] ) {

static int rows[11] = { 5,6,7,8,9,10,12,  17,20,23,26 }; // 7 CCA & 4 CCB row counts

uint8_t bitField[MAX_BYTES];
uint16_t codeWords[MAX_CW];
int size;
int i;

	linFlag = 0;
	CCSizes = CC2Sizes;
	if ((i=check2DData(str)) != 0) {
		printf("\nillegal character in 2D data = '%c'", str[i]);
		errFlag = true;
		return(0);
	}
#if PRNT
	printf("%s\n", str);
#endif
	size = pack(str, bitField);
	if (size < 0 || CC2Sizes[size] == 0) {
		printf("\ndata error");
		errFlag = true;
		return(0);
	}
	if (size <= MAX_CCA2_SIZE) {
		encCCA2(size, bitField, codeWords, pattern);
	}
	else {
		encCCB2(size-MAX_CCA2_SIZE-1, bitField, codeWords, pattern);
	}
	return(rows[size]);
}

int CC3Sizes[] = {	78,98,118,138,167,		// cca sizes
			208,304,416,536,648,768,	// ccb sizes
			0 };

int CC3enc(uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS] ) {

static int rows[11] = { 4,5,6,7,8,  15,20,26,32,38,44 }; // 5 CCA & 6 CCB row counts

uint8_t bitField[MAX_BYTES];
uint16_t codeWords[MAX_CW];
int size;
int i;

	linFlag = 0;
	CCSizes = CC3Sizes;
	if ((i=check2DData(str)) != 0) {
		printf("\nillegal character in 2D data = '%c'", str[i]);
		errFlag = true;
		return(0);
	}
#if PRNT
	printf("%s\n", str);
#endif
	size = pack(str, bitField);
	if (size < 0 || CC3Sizes[size] == 0) {
		printf("\ndata error");
		errFlag = true;
		return(0);
	}
	if (size <= MAX_CCA3_SIZE) {
		encCCA3(size, bitField, codeWords, pattern);
	}
	else {
		encCCB3(size-MAX_CCA3_SIZE-1, bitField, codeWords, pattern);
	}
	return(rows[size]);
}

static int CC4Sizes[] = {	78,108,138,167,197, // cca sizes
				208,264,352,496,672,840,1016,1184, // ccb sizes
				0 };

int CC4enc(uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS] ) {

static int rows[13] = { 3,4,5,6,7,  10,12,15,20,26,32,38,44 }; // 5 CCA & 8 CCB row counts

uint8_t bitField[MAX_BYTES];
uint16_t codeWords[MAX_CW];
int size;
int i;

	linFlag = 0;
	CCSizes = CC4Sizes;
	if ((i=check2DData(str)) != 0) {
		printf("\nillegal character in 2D data = '%c'", str[i]);
		errFlag = true;
		return(0);
	}
#if PRNT
	printf("%s\n", str);
#endif
	size = pack(str, bitField);
	if (size < 0 || CC4Sizes[size] == 0) {
		printf("\ndata error");
		errFlag = true;
		return(0);
	}
	if (size <= MAX_CCA4_SIZE) {
		encCCA4(size, bitField, codeWords, pattern);
	}
	else {
		encCCB4(size-MAX_CCA4_SIZE-1, bitField, codeWords, pattern);
	}
	return(rows[size]);
}

int CCCenc(uint8_t str[], uint8_t patCCC[] ) {

uint8_t bitField[MAX_CCC_BYTES];
uint16_t codeWords[MAX_CCC_CW];
int byteCnt;
int i;

	linFlag = -1; // CC-C flag value
	if ((i=check2DData(str)) != 0) {
		printf("illegal character '%c'\n", str[i]);
		return(false);
	}
	printf("%s\n", str);
	if((byteCnt = pack(str, bitField)) < 0) {
		printf("data error\n");
		return(false);
	}
	encCCC(byteCnt, bitField, codeWords, patCCC);
	return(true);
}


static void encCCA2(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataCw[7] = { 6,8,9,11,12,14,17 };
static int eccCw[7] = { 4,4,5,5,6,6,7 };

	encode928(bitField, codeWords, CC2Sizes[size]);

	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCA2(size, codeWords, pattern);
	return;
}

static void encCCB2(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataBytes[4] = { 26,32,37,42 };
static int dataCw[4] = { 24,29,33,37 };
static int eccCw[4] = { 10,11,13,15 };

	codeWords[0] = 920; // insert UCC/EAN flag and byte mode latch
	codeWords[1] =
		(dataBytes[size] % 6 == 0) ? 924 : 901; // 924 iff even multiple of 6
	encode900(bitField, &codeWords[2], dataBytes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCB2(size, codeWords, pattern);
	return;
}

static void encCCA3(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataCw[5] = { 8,10,12,14,17 };
static int eccCw[5] = { 4,5,6,7,7 };

	encode928(bitField, codeWords, CC3Sizes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCA3(size, codeWords, pattern);
	return;
}

static void encCCB3(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataBytes[6] = { 26,38,52,67,81,96 };
static int dataCw[6] = { 24,34,46,58,70,82 };
static int eccCw[6] = { 21,26,32,38,44,50 };

	codeWords[0] = 920; // insert UCC/EAN flag and byte mode latch
	codeWords[1] =
		(dataBytes[size] % 6 == 0) ? 924 : 901; // 924 iff even multiple of 6
	encode900(bitField, &codeWords[2], dataBytes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCB3(size, codeWords, pattern);
	return;
}

static void encCCA4(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataCw[5] = { 8,11,14,17,20 };
static int eccCw[5] = { 4,5,6,7,8 };

	encode928(bitField, codeWords, CC4Sizes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCA4(size, codeWords, pattern);
	return;
}

static void encCCB4(int size, uint8_t bitField[], uint16_t codeWords[],
		uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataBytes[8] = { 26,33,44,62,84,105,127,148 };
static int dataCw[8] = { 24,30,39,54,72,90,108,126 };
static int eccCw[8] = { 16,18,21,26,32,38,44,50 };

	codeWords[0] = 920; // insert UCC/EAN flag and byte mode latch
	codeWords[1] =
		(dataBytes[size] % 6 == 0) ? 924 : 901; // 924 iff even multiple of 6
	encode900(bitField, &codeWords[2], dataBytes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCB4(size, codeWords, pattern);
	return;
}

static void encCCC(int byteCnt, uint8_t bitField[], uint16_t codeWords[], uint8_t patCCC[]) {

int nonEccCwCnt;

	nonEccCwCnt = colCnt*rowCnt-eccCnt;
	codeWords[0] = (uint16_t)nonEccCwCnt;
	codeWords[1] = 920; // insert UCC/EAN flag and byte mode latch
	codeWords[2] =
		(byteCnt % 6 == 0) ? 924 : 901; // 924 iff even multiple of 6
	encode900(bitField, &codeWords[3], byteCnt);
	genECC(nonEccCwCnt, eccCnt, codeWords);
	imgCCC(codeWords, patCCC);
	return;
}

static void imgCCA2(int size, uint16_t codeWords[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[7] = { 5,6,7,8,9,10,12 };
static int raps[7] = { 39,1,32,8,14,43,20 };

uint32_t bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*2]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*2+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+32)%52]; // right rap (rotation 32)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

static void imgCCB2(int size, uint16_t codeWords[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[4] = { 17,20,23,26 };
static int raps[4] = { 36,19,9,27 };
static int rotate[4] = { 0,0,8,8 };

uint32_t bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*2]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*2+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+rotate[size])%52]; // right rap (rotation 0 or 8)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

static void imgCCA3(int size, uint16_t codeWords[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[5] = { 4,5,6,7,8 };
static int raps[5] = { 11,1,5,15,21 };

uint32_t bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barData[rapL%3][codeWords[i*3]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1  +j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[1][(rapL+32)%52]; // center rap (rotation 32)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1  +8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*3+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1  +8+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*3+2]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1  +8+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+32+32)%52]; // right rap (rotation 64)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1  +8+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1  +8+6+8+8+6] = 1; // right guard
		pattern[i][1  +8+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

static void imgCCB3(int size, uint16_t codeWords[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[6] = { 15,20,26,32,38,44 };
static int raps[6] = { 37,1,1,21,15,1 };
static int rotate[6] = { 0,16,8,8,16,24 };

uint32_t bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*3]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[1][(rapL+rotate[size])%52]; // center rap (rotation 0,8,16, or 24)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*3+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*3+2]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+rotate[size]*2)%52]; // right rap (double rotation)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
  }
	return;
}

static void imgCCA4(int size, uint16_t codeWords[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[5] = { 3,4,5,6,7 };
static int raps[5] = { 40,43,46,34,29 };

uint32_t bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+1]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[1][(rapL+32)%52]; // center rap (rotation 32)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+2]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+3]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+32+32)%52]; // right rap (double rotation)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+8+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

static void imgCCB4(int size, uint16_t codeWords[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[8] = { 10,12,15,20,26,32,38,44 };
static int raps[8] = { 15,25,37,1,1,21,15,1 };
static int rotate[8] = { 0,0,0,16,8,8,16,24 };

uint32_t bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+1]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[1][(rapL+rotate[size])%52]; // center rap (rotation 0,8,16, or 24)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+2]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+3]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+j] = (uint8_t)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+rotate[size]*2)%52]; // right rap (double rotation)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+8+j] = (uint8_t)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+8+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

static void imgCCC(uint16_t codeWords[], uint8_t patCCC[]) {

static uint8_t leftPtn[9] = { 2,8,1,1,1,1,1,1,3 }; // qz + start
static uint8_t rightPtn[10] = { 7,1,1,3,1,1,1,2,1,2 }; // stop + qz

int leftRowBase[3]; // right row is (left index+2) mod 3

uint32_t bars;
int cluster, errLvl, rowFactor;
int cwNdx, row, bar, offset, col;
int i;

	i = eccCnt >> 4;
	for (errLvl = 2; i > 0; i >>= 1, errLvl++); // calc ecc level
	leftRowBase[0] = (rowCnt-1)/3; // set up row indicator value bases
	leftRowBase[1] = errLvl*3 + (rowCnt-1)%3;
	leftRowBase[2] = colCnt-1;
	cwNdx = 0;
	for (cluster = row = 0; row < rowCnt; cluster = (cluster+1)%3, row++) {
		rowFactor = (row/3)*30; // row number factor for row indicators
		// left qz and start
		for (bar = 0; bar < 9; bar++) {
			patCCC[row*((colCnt+4)*8+3) + bar] = leftPtn[bar];
		}
		offset = bar;
		// left row indicator
		bars = barData[cluster][rowFactor + leftRowBase[cluster]]; // left R.I.
		for (bar = 0; bar < 8; bar++) {
			// get 8 3-bit widths left to right:
			patCCC[row*((colCnt+4)*8+3) + bar+offset] = (uint8_t)(bars >> ((7-bar)*3)) & 7;
		}
		offset += bar;
		for (col = 0; col < colCnt; col++) {
			bars = barData[cluster][codeWords[cwNdx++]]; // codeword
			for (bar = 0; bar < 8; bar++) {
				// get 8 3-bit widths left to right:
				patCCC[row*((colCnt+4)*8+3) + bar+offset] = (uint8_t)(bars >> ((7-bar)*3)) & 7;
			}
			offset += bar;
		}
		// right row indicator
		bars = barData[cluster][rowFactor + leftRowBase[(cluster+2)%3]]; // right R.I.
		for (bar = 0; bar < 8; bar++) {
			// get 8 3-bit widths left to right:
			patCCC[row*((colCnt+4)*8+3) + bar+offset] = (uint8_t)(bars >> ((7-bar)*3)) & 7;
		}
		offset += bar;
		// left qz and start
		for (bar = 0; bar < 10; bar++) {
			patCCC[row*((colCnt+4)*8+3) + bar+offset] = rightPtn[bar];
		}
  }
	return;
}

// input chars FNC1 '#' and SYM_SEP '^' also defined in iswhat array
#define FNC1 '#'
#define SYM_SEP 94

#define	NUM_MODE	1
#define	ALNU_MODE	2
#define	ISO_MODE	3
#define ALPH_MODE	4
#define	FINI_MODE	5

#define	IS_NUM		0x1
#define	IS_FNC1		0x2
#define	IS_ALNU		0x4
#define	IS_ISO		0x8
#define	IS_FINI		0x80

int pack(uint8_t str[], uint8_t bitField[] ) {

struct encodeT encode;

	encode.str = str;
	encode.bitField = bitField;
	encode.iStr = encode.iBit = 0;
	if (linFlag == 1) {
		encode.iBit++; // skip composite link bit if linear component
		encode.mode = doLinMethods(encode.str, &encode.iStr,
						encode.bitField, &encode.iBit);
	}
	else {
		encode.mode = doMethods(&encode);
	}
	while (encode.mode != FINI_MODE) {
		switch (encode.mode) {

		case NUM_MODE: {
			encode.mode = procNUM(&encode);
			break;
		}
		case ALNU_MODE: {
			encode.mode = procALNU(&encode);
			break;
		}
		case ISO_MODE: {
			encode.mode = procISO(&encode);
			break;
		}
		default: {
			printf("\nmode error");
			errFlag = true;
			return(-1);
		} } /* end of case */
	}
	if (linFlag == -1) { // CC-C
		if (!insertPad(&encode)) { // will return false if error
			printf("symbol too big\n");
			return(-1);
		}
		return(encode.iBit/8); // no error, return number of data bytes
	}
	else { // CC-A/B or RSS Exp
		return(insertPad(&encode));
	}
}

uint8_t iswhat[256] = { /* byte look up table with IS_XXX bits */
	/* 32 control characters: */
		0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	/* 32 punctuation and numeric characters: (FNC1 = # , symbol separator = ^ ) */
		8,8,8,0xf,0,8,8,8,8,8,0xc,8,0xc,0xc,0xc,0xc,
		0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd,
		8,8,8,8,8,8,
	/* 32 upper case and punctuation characters: */
		0,
		0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,
		0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,
		0,0,0,0xc,8,
	/* 32 lower case and punctuation characters: */
		0,
		8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,
		0,0,0,0,0,
	/* extended ASCII */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int check2DData(uint8_t dataStr[]) {
	int i;

	for (i = 0; iswhat[dataStr[i]] != 0x80; i++) {
		if (iswhat[dataStr[i]] == 0) {
			return(i); // error, unsupported character
		}
	}
	return(0);
}

static int procNUM(struct encodeT *encode) {

int bitCnt, char1, char2, what1, what2, i;

	// check first char type
	if ((what1 = iswhat[(char1 = encode->str[encode->iStr])]) == IS_FINI) {
		// end of data
		bitCnt = getUnusedBitCnt(encode->iBit, &i);
		if (bitCnt > 4) {
			bitCnt = 4;
		}
		if (bitCnt > 0) {
			// insert full or partial 0000 alnu latch for pad
			putBits(encode->bitField, encode->iBit, bitCnt, 0);
			encode->iBit += bitCnt;
		}
		return(FINI_MODE);
	}
	if ((what1 & IS_NUM) == 0) {
		// first not a "number", latch to ALNU
		putBits(encode->bitField, encode->iBit, 4, 0);
		encode->iBit += 4;
		return(ALNU_MODE);
	}
	// check 2nd char type
	if (((what2 = iswhat[(char2 = encode->str[(encode->iStr)+1])]) == IS_FINI) &&
			((what1 & IS_FNC1) == 0)) {
		// single digit left, check for nearly at end of bits
		encode->iStr += 1;
		bitCnt = getUnusedBitCnt(encode->iBit, &i);
		if ((bitCnt >= 4) && (bitCnt < 7)) {
			// less than 7 bits, encode a bcd+1
			putBits(encode->bitField, encode->iBit, 4, (uint16_t)(char1+1 -(int)'0'));
			bitCnt -= 4;
			if (bitCnt > 0) {
				// 0 or 00 final pad
				putBits(encode->bitField, (encode->iBit)+4, bitCnt, 0);
			}
			encode->iBit += 4 + bitCnt;
		}
		else {
			// encode as digit & FNC1
			putBits(encode->bitField, encode->iBit, 7, (uint16_t)(((char1-(int)'0') * 11) + 10 + 8));
			encode->iBit += 7;
			bitCnt -= 7;
			if ((bitCnt > 4) || (bitCnt < 0)) {
				bitCnt = 4; // either not near end or ran over to upper
			}
			if (bitCnt > 0) {
				// insert full or partial 0000 alnu latch
				putBits(encode->bitField, encode->iBit, bitCnt, 0);
				encode->iBit += bitCnt;
			}
		}
		return(FINI_MODE);
	}
	if (((what1 & what2 & IS_FNC1) != 0) || ((what2 & IS_NUM) == 0)) {
		// dbl FNC1 or 2nd char not a digit, latch to alnu
		putBits(encode->bitField, encode->iBit, 4, 0);
		encode->iBit += 4;
		return(ALNU_MODE);
	}
	else {
		// both "digits", encode as 7-bits
		encode->iStr += 2;
		if ((what1 & IS_FNC1) != 0) {
			char1 = 10;
		}
		else {
			char1 -= (int)'0';
		}
		if ((what2 & IS_FNC1) != 0) {
			char2 = 10;
		}
		else {
			char2 -= (int)'0';
		}
		putBits(encode->bitField, encode->iBit, 7, (uint16_t)((char1 * 11) + char2 + 8));
		encode->iBit += 7;
		return(NUM_MODE);
	}
}

static int procALNU(struct encodeT *encode) {

int chr, i, what, whatN;

	// check next char type
	if ((what = iswhat[(chr = encode->str[encode->iStr])]) == IS_FINI) {
		// end of data
		return(FINI_MODE);
	}
	if ((what & IS_ALNU) == 0) {
		// not a ALNU, latch to ISO
		putBits(encode->bitField, encode->iBit, 5, 4);
		encode->iBit += 5;
		return(ISO_MODE);
	}
	if (((what & IS_NUM) != 0) &&
			(((what | iswhat[encode->str[(encode->iStr)+1]]) & IS_FNC1) == 0)) {
		// next is NUM, look for more
		for (i = 1; i < 6; i++) {
			if ((whatN = iswhat[encode->str[(encode->iStr)+i]]) == IS_FINI) {
				if (i >= 4) {
					// latch numeric if >= 4 numbers at end
					putBits(encode->bitField, encode->iBit, 3, 0);
					encode->iBit += 3;
					return(NUM_MODE);
				}
				else {
					// else continue in ALNU
					break;
				}
			}
			else if ((whatN & IS_NUM) == 0) {
				// stay in ALNU if < 6 digits coming up
				break;
			}
		}
		if (i == 6) {
			// NUM if 6 or more digits coming up
			putBits(encode->bitField, encode->iBit, 3, 0);
			encode->iBit += 3;
			return(NUM_MODE);
		}
	}
	// process char in ALNU mode
	encode->iStr += 1;
	if ((what & IS_NUM) != 0) {
		// FNC1 or 0-9
		if ((what & IS_FNC1) != 0) {
			chr = 0xf;
			encode->mode = NUM_MODE;
		}
		else {
			chr = chr - (int)'0' + 5;
		}
		putBits(encode->bitField, encode->iBit, 5, (uint16_t)chr);
		encode->iBit += 5;
	}
	else {
		if (chr == SYM_SEP) {
			// ^ (symbol separator)
			chr = 0x1F;
			encode->mode = NUM_MODE;
		}
		else if (chr >= (int)'A') {
			// A-Z
			chr = chr - (int)'A';
		}
		else if (chr >= ',') {
			// ,-./
			chr = chr - (int)',' + 0x1B;
		}
		else {
			// *
			chr = 0x1A;
		}
		putBits(encode->bitField, encode->iBit, 6, (uint16_t)(chr + 0x20));
		encode->iBit += 6;
	}
	return(encode->mode);
}

static int procISO(struct encodeT *encode) {

int chr, i, what, whatN, numCnt;

	// check next char type
	if ((what = iswhat[(chr = encode->str[encode->iStr])]) == IS_FINI) {
		// end of data
		return(FINI_MODE);
	}
	numCnt = 0;
	if (((what & IS_ALNU) != 0) && ((what & IS_FNC1) == 0)) {
		// next is ALNU (& not FNC1), look 9 for more ALNU
		if ((what & IS_NUM) != 0) {
			// also count leading "digits"
			numCnt = 1;
		}
		for (i = 1; i < 10; i++) {
			whatN = iswhat[encode->str[(encode->iStr)+i]];
			if (whatN == IS_FINI) {
				if ((numCnt >= 4) || (numCnt <= -4)) {
					// latch numeric if >= 4 numbers at end
					putBits(encode->bitField, encode->iBit, 3, 0);
					encode->iBit += 3;
					return(NUM_MODE);
				}
				if (i >= 5) {
					// latch ALNU if >= 5 alphanumbers at end
					putBits(encode->bitField, encode->iBit, 5, 4);
					encode->iBit += 5;
					return(ALNU_MODE);
				}
				else {
					break;
				}
			}
			if ((whatN & IS_NUM) != 0) {
				if (numCnt > 0) {
					// count leading diits
					numCnt++;
				}
			}
			else if (numCnt > 0) {
				// stop counting if not a digit
				numCnt = -numCnt;
			}
			if ((whatN & IS_ALNU) == 0) {
				break;
			}
		}
		if (i == 10) {
			if ((numCnt >= 4) || (numCnt <= -4)) {
				// latch numeric if >= 4 numbers follow & no ISO chars in next 10
				putBits(encode->bitField, encode->iBit, 3, 0);
				encode->iBit += 3;
				return(NUM_MODE);
			}
			else {
				// latch ALNU if no ISO only chars in next 10
				putBits(encode->bitField, encode->iBit, 5, 4);
				encode->iBit += 5;
				return(ALNU_MODE);
			}
		}
	}
	// process char in ISO mode
	encode->iStr += 1;
	if ((what & IS_NUM) != 0) {
		// FNC1 or 0-9
		if ((what & IS_FNC1) != 0) {
			chr = 0xF;
			encode->mode = NUM_MODE;
		}
		else {
			chr = chr - (int)'0' + 5;
		}
		putBits(encode->bitField, encode->iBit, 5, (uint16_t)chr);
		encode->iBit += 5;
	}
	else if ((chr >= (int)'A') && (chr <= (int)'Z')) {
		// A-Z
		chr = chr - (int)'A' + 0x40;
		putBits(encode->bitField, encode->iBit, 7, (uint16_t)chr);
		encode->iBit += 7;
	}
	else if ((chr >= (int)'a') && (chr <= (int)'z')) {
		// a-z
		chr = chr - (int)'a' + 0x5A;
		putBits(encode->bitField, encode->iBit, 7, (uint16_t)chr);
		encode->iBit += 7;
	}
	else {
		if (chr == 32) {
			chr = 0xFC; // sp
		}
		else if (chr == SYM_SEP) {
			chr = 0xFD; // ^ (symbol separator)
			encode->mode = NUM_MODE;
		}
		else if (chr == 95) {
			chr = 0xFB; // _
		}
		else if (chr >= 58) {
			chr = chr - 58 + 0xF5; // :-?
		}
		else if (chr >= 37) {
			chr = chr - 37 + 0xEA; // %-/
		}
		else {
			chr = chr - 33 + 0xE8; // !-"
		}
		putBits(encode->bitField, encode->iBit, 8, (uint16_t)chr);
		encode->iBit += 8;
	}
	return(encode->mode);
}

static int procALPH(struct encodeT *encode) {

int i;

	// check next char type
	if (isupper(encode->str[encode->iStr])) {
		// alpha
		putBits(encode->bitField, encode->iBit, 5, (uint16_t)(encode->str[encode->iStr]-65));
		encode->iBit += 5;
		encode->iStr += 1;
	}
	else if (isdigit(encode->str[encode->iStr])) {
		// number
		putBits(encode->bitField, encode->iBit, 6, (uint16_t)(encode->str[encode->iStr]+4));
		encode->iBit += 6;
		encode->iStr += 1;
	}
	else if (encode->str[encode->iStr] == FNC1) {
		// FNC1
		putBits(encode->bitField, encode->iBit, 5, 31);
		encode->iBit += 5;
		encode->iStr += 1;
		encode->mode = NUM_MODE;
	}
	else {
		// '\0'
		i = getUnusedBitCnt(encode->iBit, &i);
		if (i > 5) {
			i = 5; // i is minimum of 5 or unused bit count
		}
		putBits(encode->bitField, encode->iBit, i, 31);
		encode->iBit += i;
		encode->mode = NUM_MODE;
	}
	return(encode->mode);
}

static int insertPad(struct encodeT *encode) {

int bitCnt, chr, size;

	bitCnt = getUnusedBitCnt(encode->iBit, &size);
	if (bitCnt < 0) {
		return(-1); // too many bits
	}
	while (bitCnt >= 5) {
		putBits(encode->bitField, encode->iBit, 5, 4);
		encode->iBit += 5;
		bitCnt -= 5;
	}
	if (bitCnt > 0) {
		chr = 4 >> (5-bitCnt);
		putBits(encode->bitField, encode->iBit, bitCnt, (uint16_t)chr);
		encode->iBit += bitCnt;
	}
	return(size);
}

static int doMethods(struct encodeT *encode) {

uint16_t bits;

	if (strlen((char*)encode->str) >= 8 && encode->str[0] == '1' &&
				(encode->str[1] == '1' || encode->str[1] == '7')) {
		// method "10"
		putBits(encode->bitField, 0, 2, 2); // mfg/exp date-lot encodation method bit flag 10
		bits = (uint16_t)(((uint16_t)(encode->str[2]-'0')*10 +
				(uint16_t)(encode->str[3]-'0')) * 384); // YY
		bits = (uint16_t)(bits + ((uint16_t)(encode->str[4]-'0')*10 +
				(uint16_t)(encode->str[5]-'0') - 1) * 32); // MM
		bits = (uint16_t)(bits + (uint16_t)(encode->str[6]-'0')*10 +
				(uint16_t)(encode->str[7]-'0')); // DD
		putBits(encode->bitField, 2, 16, bits); // date packed data
		putBits(encode->bitField, 2+16, 1,
				(uint16_t)((encode->str[1] == '1') ? 0 : 1)); // 0/1 bit for AI 11/17
		if (encode->str[8] == '1' && encode->str[9] == '0' &&
				encode->str[10] != '#') {
			encode->iStr = 2+6+2; // lot data follows
			encode->iBit = 2+16+1;
		}
		else {
			encode->str[7] = '#'; // insert FNC1 to indicate no lot
			encode->iStr = 2+6-1;
			encode->iBit = 2+16+1;
		}
		return(NUM_MODE);
	}
	if (encode->str[0] == '9' &&
			encode->str[1] == '0' &&
			testAI90(encode)) {
		putBits(encode->bitField, encode->iBit, 2, 3); // method 11
		encode->iBit += 2;
		procAI90(encode);
	}
	else {
		// method 0
		putBits(encode->bitField, 0, 1, 0); // g.p. encodation method bit flag 0
		encode->iBit = 1;
		encode->mode = NUM_MODE;
		encode->iStr = 0;
	}
	return(encode->mode);
}

static int testAI90(struct encodeT *encode) {

uint8_t chr;

	// possible method "11"
	// get DI number - diNum, DI alpha - diAlpha, and start of data - ndx
	encode->diNum = -1;
	if (isupper(encode->str[encode->iStr+2])) {
		encode->diAlpha = encode->str[encode->iStr+2];
		encode->diNum = 0;
		encode->iStr += 3;
	}
	else if (isdigit(encode->str[encode->iStr+2]) &&
			isupper(encode->str[encode->iStr+3])) {
		encode->diAlpha = encode->str[encode->iStr+3];
		encode->diNum = encode->str[encode->iStr+2] - '0';
		encode->iStr += 4;
	}
	else if (isdigit(encode->str[encode->iStr+2]) &&
				isdigit(encode->str[encode->iStr+3]) &&
				isupper(encode->str[encode->iStr+4])) {
		encode->diAlpha = encode->str[encode->iStr+4];
		chr = encode->str[encode->iStr+4];
		encode->str[encode->iStr+4] = '\0';
		encode->diNum = atoi((char*)&encode->str[encode->iStr+2]);
		encode->str[encode->iStr+4] = chr;
		encode->iStr += 5;
	}
	else if (isdigit(encode->str[encode->iStr+2]) &&
			isdigit(encode->str[encode->iStr+3]) &&
			isdigit(encode->str[encode->iStr+4]) &&
			isupper(encode->str[encode->iStr+5])) {
		encode->diAlpha = encode->str[encode->iStr+5];
		chr = encode->str[encode->iStr+5];
		encode->str[encode->iStr+5] = '\0';
		encode->diNum = atoi((char*)&encode->str[encode->iStr+2]);
		encode->str[encode->iStr+5] = chr;
		encode->iStr += 6;
	}
	return(encode->diNum >= 0);
}

static void procAI90(struct encodeT *encode) {

int i, j, k;
int alLessNu;
int diNum1, diAlpha1;
static uint8_t alphaTbl[] = "BDHIJKLNPQRSTVWZ"; // strlen must be 16

		// method "11", look ahead to find best compaction scheme
		j = 10000; // 10000: initial flag for non-numeric index
		alLessNu = 0; // upper-case - digit, < -9000 if non-alnu seen
		for (i = encode->iStr; i < (int)strlen((char*)encode->str); i++) {
			if ((encode->str[i] == FNC1) || (encode->str[i] == SYM_SEP)) {
				break; // found it
			}
			if (j == 10000 && !isdigit(encode->str[i])) {
				j = i; // save first non-numeric index
			}
			if (isdigit(encode->str[i])) {
				alLessNu--;
			}
			else if (isupper(encode->str[i])) {
				alLessNu++;
			}
			else {
				alLessNu = -10000; // flag that char not digit or upper seen
			}
		}
		diNum1 = encode->diNum; // save in case nextAI called
		diAlpha1 = encode->diAlpha; // save in case nextAI called
		if (encode->str[i] == FNC1) {
			k = encode->iStr;
			encode->iStr = i+1;
			nextAI(encode);
			encode->iStr = k;
		}
		else {
			encode->typeAI = AIx;
		}
		// do encodation bit(s)
		if (alLessNu > 0) {
			putBits(encode->bitField, encode->iBit, 2, 3); // 11: alpha encoding
			encode->iBit += 2;
			encode->mode = ALPH_MODE;
		}
		else if (i > j && j-encode->iStr < 4) {
			putBits(encode->bitField, encode->iBit, 1, 0); // 0: alphanumeric encoding
			encode->iBit += 1;
			encode->mode = ALNU_MODE;
		}
		else {
			putBits(encode->bitField, encode->iBit, 2, 2); // 10: numeric encoding
			encode->iBit += 2;
			encode->mode = NUM_MODE;
		}
		// next AI is 1 or 2 bit field
		if (encode->typeAI == AIx) {
			putBits(encode->bitField, encode->iBit, 1, 0); // 0: not AI 21 or 8004
			encode->iBit += 1;
		}
		else { // 10: AI 21 or 11: AI 8004
			putBits(encode->bitField, encode->iBit, 2, (uint16_t)encode->typeAI);
			encode->iBit += 2;
		}
		for (j = 0; j < 16; j++) {
			if (diAlpha1 == alphaTbl[j]) {
				break;
			}
		}
		if (diNum1 < 31 && j < 16) {
			putBits(encode->bitField, encode->iBit, 5, (uint16_t)diNum1); // DI number < 31
			putBits(encode->bitField,
					encode->iBit+5, 4, (uint16_t)j); // DI alpha from alphaTbl
			encode->iBit += 9;
		}
		else {
			putBits(encode->bitField, encode->iBit, 5, 31);
			putBits(encode->bitField,
					encode->iBit+5, 10, (uint16_t)diNum1); // DI number >= 31
			putBits(encode->bitField,
					encode->iBit+15, 5, (uint16_t)(diAlpha1-65)); // or alpha not in table
			encode->iBit += 20;
		}
		encodeAI90(encode);
		if (encode->typeAI == AI21) {
			encode->iStr += 2; // skip "21"
		}
		else if (encode->typeAI == AI8004) {
			encode->iStr += 4; // skip "8004"
		}
	return;
}

static void encodeAI90(struct encodeT *encode) {

	while ((encode->str[encode->iStr-1] != FNC1) &&
					(encode->str[encode->iStr-2] != FNC1) &&
					(encode->str[encode->iStr-1] != SYM_SEP) &&
					(encode->mode != FINI_MODE)) {
		switch (encode->mode) {
		case NUM_MODE: {
			if (encode->str[encode->iStr] == FNC1) {
				if (encode->typeAI == AI21) {
					// move up char after "21" in case it is needed for NUM_MODE
					encode->str[encode->iStr+1] = encode->str[encode->iStr+3];
				}
				else if (encode->typeAI == AI8004) {
					// move up char after "8004" in case it is needed for NUM_MODE
					encode->str[encode->iStr+1] = encode->str[encode->iStr+5];
				}
			}
			encode->mode = procNUM(encode);
			break;
		}
		case ALNU_MODE: {
			encode->mode = procALNU(encode);
			break;
		}
		case ISO_MODE: {
			encode->mode = procISO(encode);
			break;
		}
		case ALPH_MODE: {
			encode->mode = procALPH(encode);
			break;
		}
		default: {
			printf("\nmode error");
			errFlag = true;
			return;
		} } /* end of case */
	}
	return;
}

static void nextAI(struct encodeT *encode) {

	if (encode->str[encode->iStr+0] == '2' &&
			encode->str[encode->iStr+1] == '1') {
		encode->typeAI = AI21;
		encode->iStr += 2;
	}
	else if (encode->str[encode->iStr+0] == '8' &&
			encode->str[encode->iStr+1] == '0' &&
			encode->str[encode->iStr+2] == '0' &&
			encode->str[encode->iStr+3] == '4') {
		encode->typeAI = AI8004;
		encode->iStr += 4;
	}
	else {
		encode->typeAI = AIx; // not AI 21 or 8004
	}
	return;
}

static int doLinMethods(uint8_t str[], int *iStr, uint8_t bitField[], int *iBit) {

uint16_t bits;
long weight;
char numStr[10] = { 0 };

	if (strlen((char*)str) >= 26) {
		strncpy(numStr, (char*)&str[20], 6); // possible weight field
		numStr[6] = '\0';
	}
	// look for AI 01
	if (strlen((char*)str)>=16 && str[0]=='0' && str[1]=='1') {

		// look for fixed length with AI 01[9] + 3103[0-32767]
		if (str[2]=='9' && (strlen((char*)str)==16+10) &&
			str[16]=='3' && str[17]=='1' && str[18]=='0' && str[19]=='3' &&
			(weight=atol(numStr))<=32767L) {
			// method 0100, AI's 01 + 3103
			putBits(bitField, *iBit, 4, 4); // write method
			*iBit += 4;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 15, (uint16_t)weight); // write weight
			*iBit += 15;
			*iStr += 1+10; // skip check digit & jump weight field
		}

		// look for fixed length with AI 01[9] + 3202[0-009999]
		else if (str[2]=='9' && (strlen((char*)str)==16+10) &&
			str[16]=='3' && str[17]=='2' && str[18]=='0' && str[19]=='2' &&
			(weight=atol(numStr))<=9999L) {
			// method 0101, AI's 01 + 3202
			putBits(bitField, *iBit, 4, 5); // write method
			*iBit += 4;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 15, (uint16_t)weight); // write weight
			*iBit += 15;
			*iStr += 1+10; // skip check digit & jump weight field
		}

		// look for fixed length with AI 01[9] + 3203[0-022767]
		else if (str[2]=='9' && (strlen((char*)str)==16+10) &&
			str[16]=='3' && str[17]=='2' && str[18]=='0' && str[19]=='3' &&
			(weight=atol(numStr))<=22767L) {
			// method 0101, AI's 01 + 3203
			putBits(bitField, *iBit, 4, 5); // write method
			*iBit += 4;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 15, (uint16_t)(weight+10000)); // write weight
			*iBit += 15;
			*iStr += 1+10; // skip check digit & jump weight field
		}

		// look for AI 01[9] + 392[0-3]
		else if (str[2]=='9' && (strlen((char*)str)>=16+4+1) &&
			str[16]=='3' && str[17]=='9' && str[18]=='2' &&
			(str[19]>='0' && str[19]<='3')) {
			// method 01100, AI's 01 + 392x + G.P.
			putBits(bitField, *iBit, 5+2, 0x0C<<2); // write method + 2 VLS bits
			*iBit += 5+2;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 2, (uint16_t)(str[19]-'0')); // write D.P.
			*iBit += 2;
			*iStr += 1+4; // skip check digit & jump price AI
		}

		// look for AI 01[9] + 393[0-3]
		else if (str[2]=='9' && (strlen((char*)str)>=16+4+3+1) &&
			str[16]=='3' && str[17]=='9' && str[18]=='3' &&
			(str[19]>='0' && str[19]<='3')) {
			// method 01101, AI's 01 + 393x[NNN] + G.P.
			putBits(bitField, *iBit, 5+2, 0x0D<<2); // write method + 2 VLS bits
			*iBit += 5+2;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 2, (uint16_t)(str[19]-'0')); // write D.P.
			*iBit += 2;
			*iStr += 1+4; // skip check digit & jump price AI
			strncpy(numStr, (char*)&str[20], 3); // ISO country code
			numStr[3] = '\0';
			putBits(bitField, *iBit, 10, (uint16_t)atoi(numStr)); // write ISO c.c.
			*iBit += 10;
			*iStr += 3; // jump ISO country code
		}

		// look for fixed length with AI 01[9] + 310x/320x[0-099999]
		else if (str[2]=='9' && (strlen((char*)str)==16+10) &&
			str[16]=='3' && (str[17]=='1' || str[17]=='2') && str[18]=='0' &&
			(weight=atol(numStr))<=99999L) {
			// methods 0111000-0111001, AI's 01 + 3x0x no date
			bits = (uint16_t)(0x38+(str[17]-'1'));
			putBits(bitField, *iBit, 7, bits); // write method
			*iBit += 7;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			weight = weight + ((long)(str[19] - '0') * 100000L); // decimal digit
			putBits(bitField, *iBit, 4, (uint16_t)(weight>>16)); // write weight
			putBits(bitField, *iBit+4, 16, (uint16_t)(weight&0xFFFF));
			*iBit += 20;
			*iStr += 1+10; // jump check digit and weight field
			putBits(bitField, *iBit, 16, (uint16_t)38400); // write no date
			*iBit += 16;
		}

		// look for fixed length + AI 01[9] + 310x/320x[0-099999] + 11/13/15/17
		else if (str[2]=='9' && strlen((char*)str)==16+10+8 &&
			str[16]=='3' && (str[17]=='1' || str[17]=='2') && str[18]=='0' &&
			(weight=atol(numStr))<=99999L &&
			str[26]=='1' &&
			(str[27]=='1' || str[27]=='3' || str[27]=='5' || str[27]=='7')) {
			// methods 0111000-0111111, AI's 01 + 3x0x + 1x
			bits = (uint16_t)(0x38+(str[27]-'1')+(str[17] - '1'));
			putBits(bitField, *iBit, 7, bits); // write method
			*iBit += 7;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			weight = weight + ((long)(str[19] - '0') * 100000L); // decimal digit
			putBits(bitField, *iBit, 4, (uint16_t)(weight>>16)); // write weight
			putBits(bitField, *iBit+4, 16, (uint16_t)(weight&0xFFFF));
			*iBit += 20;
			*iStr += 11; // jump check digit & weight field
			putBits(bitField, *iBit, 16, yymmdd(&str[*iStr+2])); // write date
			*iBit += 16;
			*iStr += 8; // date field
		}
		else {
			// method 1 (plus 2-bit variable lng sym bit fld), AI 01
			putBits(bitField, *iBit, 1+2, 1<<2); // write method + 2 VLS bits
			*iBit += 1+2;
			*iStr += 2;
			cnv13(str, iStr, bitField, iBit);
			*iStr += 1; //skip check digit
		}
	}
	else {
		// method 00 (plus 2-bit variable lng sym bit fld), not AI 01
		putBits(bitField, *iBit, 2+2, 0); // write method + 2 VLSB bits
		*iBit += 2+2;
	}
	return(NUM_MODE);
}

static uint16_t yymmdd(uint8_t str[]) {

uint16_t val;

	val = (uint16_t)(((uint16_t)(str[0]-'0')*10 + (uint16_t)(str[1]-'0')) * 384); // YY
	val = (uint16_t)(val + ((uint16_t)(str[2]-'0')*10 + (uint16_t)(str[3]-'0') - 1) * 32); // MM
	val = (uint16_t)(val + (uint16_t)(str[4]-'0')*10 + (uint16_t)(str[5]-'0')); // DD
	return(val);
}

// converts 13 digits to 44 bits
static void cnv13 (uint8_t str[], int *iStr, uint8_t bitField[], int *iBit) {

int i;

	putBits(bitField, *iBit, 4, (uint16_t)(str[*iStr] - '0')); // high order 4 bits
	*iBit += 4;
	*iStr += 1;
	for (i = 0; i < 4 ; i++) {
		putBits(bitField, *iBit, 10, (uint16_t)((uint16_t)(str[*iStr] - '0')*100 +
							(str[*iStr+1] - '0')*10 +
							str[*iStr+2] - '0')); // 10 bit groups bits
		*iBit += 10;
		*iStr += 3;
	}
	return;
}

// converts 12 digits to 40 bits
static void cnv12 (uint8_t str[], int *iStr, uint8_t bitField[], int *iBit) {
int i;

	for (i = 0; i < 4 ; i++) {
		putBits(bitField, *iBit, 10, (uint16_t)((uint16_t)(str[*iStr] - '0')*100 +
				(str[*iStr+1] - '0')*10 +
				str[*iStr+2] - '0')); // 10 bit groups bits
		*iBit += 10;
		*iStr += 3;
	}
	return;
}

/*
returns number of bits left to closest 2d symbol,
 or -1 if iBit is larger than largest in type and width
 also returns CCSizes index or data char cnt for RSS14E in *size
*/
static int getUnusedBitCnt(int iBit, int *size) {

// max data plus ecc codewords for CC-C
static int eccMaxCW[] = { 40+8, 160+16, 320+32, 863+64, 0 };

int i, byteCnt, cwCnt;

	if (linFlag == 1) { // RSS Expanded
		*size = 0;
		if (iBit <= 252) {
			if ((*size = (iBit + 11) / 12) < 3) {
				*size = 3; // 3 data sym chars minimum
			}
			if ((((*size)+1+rowWidth) % rowWidth) == 1) {
				(*size)++; // last row minimum of 2
			}
			return(*size*12 - iBit);
		}
	}
	else if (linFlag == 0) { // CC-A/B
		for (i = 0; CCSizes[i] != 0; i++) {
			if (iBit <= CCSizes[i]) {
				*size = i;
				return(CCSizes[i] - iBit);
			}
		}
	}
	else if (linFlag == -1) { // CC-C
		*size = false; // size used as error flag for CC-C
		// calculate cwCnt from # of bits
		byteCnt = (iBit+7)/8;
		i = byteCnt/6;
		cwCnt = (byteCnt/6)*5 + byteCnt%6;
		// find # of ecc codewords
		for (i = 0, eccCnt = 8; eccCnt <= 64; i++, eccCnt *= 2) {
			if (cwCnt + eccCnt <= eccMaxCW[i]) {
				break;
			}
		}
		if (eccCnt > 64) {
			return(-1); // too many codewords for CCC
		}

		colCnt++; // preadjust for first decrement in loop
		do {
			colCnt--; // make narrower until satisfies min aspect ratio
			rowCnt = max(3, (1 + 2 + cwCnt + eccCnt + colCnt-1) / colCnt);
			if (rowCnt > MAX_CCC_ROWS) {
				return(-1); // too many rows for CCC
			}
		} while (colCnt + 4 > rowCnt*4);

		if (rowCnt == 3) { // find minimum width if 3 rows, but no less than 4 data)
			colCnt = max(4, (1 + 2 + cwCnt + eccCnt + 2) / 3);
		}
		cwCnt = colCnt*rowCnt - 1 - 2 - eccCnt;
		byteCnt = (cwCnt/5)*6 + cwCnt%5;
		*size = true;
		return(byteCnt*8 - iBit);
	}
	return(-1);
}

void putBits(uint8_t bitField[], int bitPos, int length, uint16_t bits) {
	int i, maxBytes;

	if (linFlag == -1) {
		maxBytes = MAX_CCC_BYTES; // CC-C
	}
	else {
		maxBytes = MAX_BYTES; // others
	}
	if ((bitPos+length > maxBytes*8) || (length > 16)) {
		printf("\nputBits error, %d, %d\n", bitPos, length);
		errFlag = true;
		return;
	}
	for (i = length-1; i >= 0; i--) {
		if ((bits & 1) != 0) {
			bitField[(bitPos+i)/8] = (uint8_t)(bitField[(bitPos+i)/8] | (0x80 >> ((bitPos+i)%8)));
		}
		else {
			bitField[(bitPos+i)/8] = (uint8_t)(bitField[(bitPos+i)/8] & (~(0x80 >> ((bitPos+i)%8))));
		}
		bits >>= 1;
	}
	return;
}

static uint16_t pwr928[69][7];

/* initialize pwr928 encoding table */
void init928(void) {

int i, j, v;
int cw[7];

	cw[6] = 1L;
	for (i = 5; i >= 0; i--) {
		cw[i] = 0;
	}
	for (i = 0; i < 7; i++) pwr928[0][i] = (uint16_t)cw[i];
	for (j = 1; j < 69; j++) {
		for (v = 0, i = 6; i >= 1; i--) {
			v = (2 * cw[i]) + (v / 928);
			cw[i] = v % 928;
			pwr928[j][i] = (uint16_t)(v % 928);
		}
		cw[0] = (2 * cw[0]) + (v / 928);
		pwr928[j][0] = (uint16_t)((2 * cw[0]) + (v / 928));
	}
	return;
}

/* converts bit string to base 928 values, codeWords[0] is highest order */
static int encode928(uint8_t bitString[], uint16_t codeWords[], int bitLng) {

int i, j, b, bitCnt, cwNdx, cwCnt, cwLng;

	for (cwNdx = cwLng = b = 0; b < bitLng; b += 69, cwNdx += 7) {
		bitCnt = min(bitLng-b, 69);
		cwLng += cwCnt = bitCnt/10 + 1;
		for (i = 0; i < cwCnt; i++) codeWords[cwNdx+i] = 0; /* init 0 */
		for (i = 0; i < bitCnt; i++) {
			if (getBit(bitString, b+bitCnt-i-1)) {
				for (j = 0; j < cwCnt; j++) {
					codeWords[cwNdx+j] = (uint16_t)(codeWords[cwNdx+j] + pwr928[i][j+7-cwCnt]);
				}
			}
		}
		for (i = cwCnt-1; i > 0; i--) {
			/* add "carries" */
			codeWords[cwNdx+i-1] = (uint16_t)(codeWords[cwNdx+i-1] + codeWords[cwNdx+i]/928);
			codeWords[cwNdx+i] %= 928;
		}
	}
	return(cwLng);
}

/* converts bytes to base 900 values (codeWords), codeWords[0] is highest order */
static void encode900(uint8_t byteArr[], uint16_t codeWords[], int byteLng) {

static uint16_t pwrByte[6][5] = { {0,0,0,0,1}, {0,0,0,0,256}, {0,0,0,72,736},
					{0,0,20,641,316}, {0,5,802,385,796}, {1,608,221,686,376}  };

int i, j, bCnt, cwNdx;
uint32_t cw, t, carry, cwArr[5];

	for (cwNdx = bCnt = 0; bCnt < byteLng-5; cwNdx += 5, bCnt += 6) {
		// init cwArr to 6th byte
		for (i = 0; i < 4; i++) cwArr[i] = 0L;
		cwArr[4] = (uint32_t)byteArr[bCnt + 5];
		for (i = 4; i >= 0; i--) {
	  	// add in 5th thru 1st bytes multilpied by pwrByte subarry
			cw = (uint32_t)byteArr[bCnt + i];
			carry = 0L;
			for (j = 4; j >= 0; j--) {
				t = cwArr[j] + cw * (uint32_t)pwrByte[5-i][j] + carry;
				carry = t / 900L;
				cwArr[j] = t % 900L;
			}
		}
		// transfer 5 cwArr packed data to codeWords
		for (i = 0; i < 5; i++) {
			codeWords[cwNdx + i] = (uint16_t)cwArr[i];
		}
	}
	// transfer 5 or less remaining cwArr to codeWords as is
	for (i = 0; i < byteLng - bCnt; i++) {
		codeWords[cwNdx + i] = byteArr[bCnt + i];
	}
	return;
}



/* gets bit in bitString at bitPos */
static int getBit(uint8_t bitStr[], int bitPos) {
		return(((bitStr[bitPos/8] & (0x80>>(bitPos%8))) == 0) ?	0 : 1);
}

/* GF(929) log and antilog tables: */
int gfPwr[928];
int gfLog[929];
int gpa[512];

static void genECC(int dsize, int csize, uint16_t sym[]) {
	int i, n, t;

	genPoly(csize);

	/* first zero ecc words */
	for (i = dsize; i < dsize+csize; i++) {
		sym[i] = 0;
	}
	/* generate check characters */
	for ( n = 0; n < dsize; n++ ) {
		t = (sym[dsize] + sym[n]) % 929;
		for (i = 0; i < csize-1; i++) {
			sym[dsize+i] = (uint16_t)(sym[dsize+i+1] + 929 - gfMul(t, gpa[csize-1 - i])) % 929;
		}
		sym[dsize+csize-1] = (uint16_t)(929 - gfMul(t, gpa[0])) % 929;
	}
	for (i = dsize; i < dsize+csize; i++) {
		sym[i] = (uint16_t)(929 - sym[i]) % 929;
	}
	return;
}

static void genPoly(int eccSize) {
	int	i, j;

	gpa[0] = 1;
	for (i = 1; i < eccSize+1; i ++) { gpa[i] = 0; }
	for (i = 0; i < eccSize; i++ ) {
		for (j = i; j >= 0; j --) {
			gpa[j+1] = (gpa[j] + gfMul(gpa[j+1], gfPwr[i+1])) % 929;
		}
		gpa[0] = gfMul(gpa[0], gfPwr[i+1]);
	}
	for (i = eccSize-1; i >= 0; i-=2 ) {
		gpa[i] = (929 - gpa[i]) % 929;
	}
}

static int gfMul(int a,int b) {
	if ((a == 0) || (b == 0)) return(0);
	return(gfPwr[(gfLog[a] + gfLog[b]) % 928]);
}

void initLogTables(void) {
	int i, j;

	for (j = 0; j < 929; j++) { gfLog[j] = 0; }
	i = 1;
	for (j = 0; j < 928; j++) {
		gfPwr[j] = i;
		gfLog[i] = j;
		i = (i * 3) % 929;
	}
	return;
}

const uint32_t barData[3][929] = {{
 6591070,8688228,10785386,6591133,8688291,10785449,4494038,6591196,4494101,2397006,
 4494164,2397069,4494430,6591588,8688746,4494493,6591651,8688809,2397398,4494556,
 2397461,2397790,4494948,6592106,2397853,4495011,6592169,2397916,4495074,2398308,
 4495466,2398371,4495529,2398826,10787913,6595165,8692323,10789481,4498070,6595228,
 8692386,4498133,6595291,2401038,4498196,6595354,2401101,4498259,2401164,4498525,
 6595683,8692841,2401430,4498588,6595746,2401493,4498651,6595809,2401556,4498714,
 2401885,4499043,6596201,2401948,4499106,2402011,4499169,2402403,4499561,2402466,
 2402529,4502102,6599260,8696418,4502165,6599323,8696481,2405070,4502228,6599386,
 2405133,4502291,2405196,2405259,2405462,4502620,6599778,2405525,4502683,6599841,
 2405588,4502746,2405651,2405714,2405980,4503138,2406043,4503201,2406106,2406498,
 4506197,6603355,8700513,2409102,4506260,6603418,2409165,4506323,6603481,2409228,
 4506386,2409291,4506449,2409557,4506715,6603873,2409620,4506778,2409683,4506841,
 2409746,2409809,4507233,2410201,2413134,4510292,6607450,2413197,4510355,6607513,
 2413260,4510418,2413323,4510481,2413386,2413652,2413715,2413778,2417229,6611545,
 4514450,4514513,2417481,6853213,8950371,11047529,4756118,6853276,8950434,4756181,
 6853339,8950497,2659086,4756244,2659149,4756573,6853731,8950889,2659478,4756636,
 6853794,2659541,4756699,2659604,2659667,2659933,4757091,6854249,2659996,4757154,
 2660059,2660122,2660451,4757609,2660514,2660969,6623830,8720988,10818146,6623893,
 8721051,10818209,4526798,6623956,8721114,4526861,6624019,8721177,4526924,6624082,
 4760150,6857308,8954466,4527190,4760213,6857371,8954529,4527253,6624411,8721569,
 2430158,2663181,4760339,6857497,2430221,4527379,2430284,2663510,4760668,6857826,
 2430550,2663573,4760731,6857889,2430613,4527771,6624929,2430676,2663699,2430739,
 2664028,4761186,2431068,2664091,4761249,2431131,4528289,2431194,2664546,2431586,
 2664609,2431649,6627925,8725083,10822241,4530830,6627988,8725146,4530893,6628051,
 8725209,4530956,6628114,4531019,4531082,4764245,6861403,8958561,4531285,4764308,
 6861466,2434190,2667213,6628506,6861529,2434253,2667276,4764434,2434316,4531474,
 4764497,2667402,2667605,4764763,6861921,2434645,2667668,4764826,2434708,4531866,
 4764889,2434771,2667794,2667857,2668123,4765281,2435163,2668186,2435226,2668249,
 2435289,2435681,4534862,6632020,8729178,4534925,6632083,8729241,4534988,6632146,
 4535051,6632209,4535114,4535177,2671182,4768340,6865498,2438222,2671245,4768403,
 6865561,2438285,4535443,6632601,2438348,2671371,4768529,2438411,4535569,2438474,
 2671700,4768858,2438740,2671763,4768921,2438803,4535961,2438866,2671889,2438929,
 2439258,2439321,4538957,6636115,8733273,4539020,6636178,4539083,6636241,4539146,
 4539209,2675277,4772435,6869593,2442317,2675340,4772498,2442380,4539538,4772561,
 2442443,2675466,2442506,2675529,2675795,2442835,2442898,2442961,6640210,6640273,
 4543241,4776530,2679435,2446475,2446538,2446601,5018198,7115356,9212514,5018261,
 7115419,2921166,5018324,7115482,2921229,5018387,2921292,2921355,2921558,5018716,
 7115874,2921621,5018779,7115937,2921684,5018842,2921747,2921810,2922076,5019234,
 2922139,5019297,2922202,2922594,2922657,6885973,8983131,11080289,4788878,6886036,
 8983194,4788941,6886099,8983257,4789004,6886162,4789067,6886225,5022293,7119451,
 9216609,4789333,5022356,7119514,2692238,2925261,6886554,7119577,2692301,4789459,
 5022482,2692364,2925387,2692427,2925653,5022811,7119969,2692693,2925716,6887009,
 2692756,4789914,5022937,2692819,2925842,2692882,2926171,5023329,2693211,2926234,
 2693274,2926297,2926689,2693729,6656590,8753748,10850906,6656653,8753811,10850969,
 6656716,8753874,6656779,8753937,6656842,4792910,6890068,8987226,4559950,4792973,
 8754266,8987289,4560013,6657171,8754329,4560076,4793099,6890257,4560139,6657297,
 4793225,2929230,5026388,7123546,2696270,2929293,5026451,7123609,2463310,2696333,
 4793491,6890649,2463373,4560531,6657689,5026577,2463436,2696459,4793617,2463499,
 2929545,2929748,5026906,2696788,2929811,5026969,2463828,2696851,4794009,2463891,
 4561049,2929937,2696977,2930266,2697306,2930329,2464346,2697369,6660685,8757843,
 10855001,6660748,8757906,6660811,8757969,6660874,6660937,4797005,6894163,8991321,
 4564045,4797068,8758361,4564108,6661266,6894289,4564171,4797194,4564234,4797257,
 4564297,2933325,5030483,7127641,2700365,2933388,5030546,2467405,2700428,4797586,
 5030609,2467468,4564626,2933514,2467531,2700554,2933577,2700617,2933843,5031001,
 2700883,2933906,2467923,2700946,2933969,2467986,2701009,2934361,2468441,6664780,
 8761938,6664843,8762001,6664906,6664969,4801100,6898258,4568140,4801163,6898321,
 4568203,6665361,4568266,4801289,4568329,2937420,5034578,2704460,2937483,5034641,
 2471500,2704523,4801681,2471563,4568721,2937609,2471626,2704649,2471689,2704978,
 2472018,2472081,8766033,6669001,6902353,4805258,4805321,2941515,2708555,2475595,
 2941641,2708681,5280341,3183246,5280404,3183309,5280467,7377625,3183372,5280530,
 3183435,5280593,3183701,5280859,7378017,3183764,5280922,3183827,5280985,3183890,
 3183953,3184219,5281377,3184282,3184345,3184737,5050958,7148116,9245274,5051021,
 7148179,5051084,7148242,5051147,7148305,5051210,3187278,5284436,7381594,2954318,
 3187341,7148634,7381657,2954381,5051539,7148697,2954444,3187467,5284625,2954507,
 5051665,3187593,3187796,5284954,2954836,3187859,5285017,2954899,5052057,2954962,
 3187985,3188314,2955354,3188377,2955417,6918733,9015891,11113049,6918796,9015954,
 6918859,9016017,6918922,6918985,5055053,7152211,4822093,5055116,7152274,4822156,
 6919314,7152337,4822219,5055242,4822282,5055305,4822345,3191373,5288531,2958413,
 3191436,7152729,2725453,2958476,5055634,5288657,2725516,4822674,3191562,2725579,
 2958602,3191625,2958665,3191891,5289049,2958931,3191954,2725971,2958994,3192017,
 2726034,2959057,3192409,2959449,2726489,8786508,10883666,8786571,10883729,8786634,
 8786697,6922828,9019986,6689868,8787026,9020049,6689931,8787089,6689994,6923017,
 6690057,5059148,7156306,4826188,5059211,7156369,4593228,4826251,6923409,4593291,
 6690449,5059337,4593354,4826377,4593417,3195468,5292626,2962508,3195531,5292689,
 2729548,2962571,5059729,2496588,2729611,4826769,3195657,2496651,4593809,2962697,
 2496714,3195986,2963026,3196049,2730066,2963089,2497106,2730129,2497169,8790603,
 10887761,8790666,8790729,6926923,9024081,6693963,8791121,6694026,6927049,6694089,
 5063243,7160401,4830283,5063306,4597323,4830346,5063369,4597386,4830409,4597449,
 3199563,5296721,2966603,5063761,2733643,2966666,3199689,2500683,2733706,2966729,
 2500746,2733769,2500809,2967121,2501201,8794761,6698058,6698121,4834378,4601418,
 4601481,2970698,2737738,2504778,2504841,3445326,3445389,5542547,3445452,3445515,
 3445578,3445844,3445907,3445970,3446033,3446362,3446425,5313101,5313164,7410322,
 5313227,7410385,5313290,5313353,3449421,5546579,3216461,5313619,5546642,3216524,
 5313682,3216587,5313745,3216650,3449673,3216713,3449939,5547097,3216979,5314137,
 3217042,3450065,3217105,3450457,3217497,7180876,7180939,7181002,7181065,5317196,
 5084236,7181394,7414417,5084299,7181457,5084362,5317385,5084425,3453516,3220556,
 3453579,5550737,2987596,3220619,3453642,2987659,3220682,3453705,2987722,3220745,
 3454034,3221074,3454097,2988114,3221137,2988177,9048651,9048714,9048777,7184971,
 6952011,9049169,6952074,7185097,6952137,5321291,7418449,5088331,7185489,4855371,
 5088394,5321417,4855434,5088457,4855497,3457611,5554769,3224651,5321809,2991691,
 3224714,3457737,2758731,2991754,3224777,2758794,2991817,3458129,3225169,2992209,
 2759249,10916426,10916489,9052746,8819786,9052809,8819849,7189066,6956106,7189129,
 6723146,6956169,6723209,5325386,5092426,5325449,4859466,5092489,4626506},{
 10785365,12882523,8688270,10785428,12882586,8688333,10785491,12882649,8688396,10785554,
 8688459,10785617,8688522,8688725,10785883,12883041,6591630,8688788,10785946,6591693,
 8688851,10786009,6591756,8688914,6591819,8688977,6591882,6592085,8689243,10786401,
 4494990,6592148,8689306,4495053,6592211,8689369,4495116,6592274,4495179,6592337,
 4495242,4495445,6592603,8689761,2398350,4495508,6592666,2398413,4495571,6592729,
 2398476,4495634,2398539,4495697,2398805,4495963,6593121,2398868,4496026,2398931,
 4496089,2398994,2399323,4496481,2399386,2399449,8692302,10789460,12886618,8692365,
 10789523,12886681,8692428,10789586,8692491,10789649,8692554,8692617,6595662,8692820,
 10789978,6595725,8692883,10790041,6595788,8692946,6595851,8693009,6595914,6595977,
 4499022,6596180,8693338,4499085,6596243,8693401,4499148,6596306,4499211,6596369,
 4499274,4499337,2402382,4499540,6596698,2402445,4499603,6596761,2402508,4499666,
 2402571,4499729,2402634,2402900,4500058,2402963,4500121,2403026,2403089,2403418,
 2403481,8696397,10793555,12890713,8696460,10793618,8696523,10793681,8696586,8696649,
 6599757,8696915,10794073,6599820,8696978,6599883,8697041,6599946,6600009,4503117,
 6600275,8697433,4503180,6600338,4503243,6600401,4503306,4503369,2406477,4503635,
 6600793,2406540,4503698,2406603,4503761,2406666,2406729,2406995,4504153,2407058,
 2407121,2407513,8700492,10797650,8700555,10797713,8700618,8700681,6603852,8701010,
 6603915,8701073,6603978,6604041,4507212,6604370,4507275,6604433,4507338,4507401,
 2410572,4507730,2410635,4507793,2410698,2410761,2411090,2411153,8704587,10801745,
 8704650,8704713,6607947,8705105,6608010,6608073,4511307,6608465,4511370,4511433,
 2414667,4511825,2414730,2414793,8708682,8708745,6612042,6612105,4515402,4515465,
 8950350,11047508,13144666,8950413,11047571,13144729,8950476,11047634,8950539,11047697,
 8950602,8950665,6853710,8950868,11048026,6853773,8950931,11048089,6853836,8950994,
 6853899,8951057,6853962,6854025,4757070,6854228,8951386,4757133,6854291,8951449,
 4757196,6854354,4757259,6854417,4757322,4757385,2660430,4757588,6854746,2660493,
 4757651,6854809,2660556,4757714,2660619,4757777,2660682,2660948,4758106,2661011,
 4758169,2661074,2661137,2661466,2661529,10818125,12915283,2429556,10818188,12915346,
 2429619,10818251,12915409,2429682,10818314,10818377,8954445,11051603,13148761,8721485,
 8954508,12915801,8721548,10818706,11051729,8721611,8954634,8721674,8954697,8721737,
 6857805,8954963,11052121,6624845,6857868,8955026,6624908,8722066,8955089,6624971,
 6857994,6625034,6858057,6625097,4761165,6858323,8955481,4528205,4761228,6858386,
 4528268,6625426,6858449,4528331,4761354,4528394,4761417,4528457,2664525,4761683,
 6858841,2431565,2664588,4761746,2431628,4528786,4761809,2431691,2664714,2431754,
 2664777,2665043,4762201,2432083,2665106,2432146,2665169,2432209,2665561,10822220,
 12919378,2433651,10822283,12919441,2433714,10822346,2433777,10822409,8958540,11055698,
 8725580,8958603,11055761,8725643,10822801,8725706,8958729,8725769,6861900,8959058,
 6628940,6861963,8959121,6629003,8726161,6629066,6862089,6629129,4765260,6862418,
 4532300,4765323,6862481,4532363,6629521,4532426,4765449,4532489,2668620,4765778,
 2435660,2668683,4765841,2435723,4532881,2435786,2668809,2435849,2669138,2436178,
 2669201,2436241,10826315,12923473,2437746,10826378,2437809,10826441,8962635,11059793,
 8729675,10826833,8729738,8962761,8729801,6865995,8963153,6633035,6866058,6633098,
 6866121,6633161,4769355,6866513,4536395,6633553,4536458,4769481,4536521,2672715,
 4769873,2439755,2672778,2439818,2672841,2439881,2673233,2440273,10830410,2441841,
 10830473,8966730,8733770,8966793,8733833,6870090,6637130,6870153,6637193,4773450,
 4540490,4773513,4540553,2676810,2443850,2676873,2443913,10834505,8970825,8737865,
 6874185,6641225,4777545,4544585,9212493,11309651,13406809,9212556,11309714,9212619,
 11309777,9212682,9212745,7115853,9213011,11310169,7115916,9213074,7115979,9213137,
 7116042,7116105,5019213,7116371,9213529,5019276,7116434,5019339,7116497,5019402,
 5019465,2922573,5019731,7116889,2922636,5019794,2922699,5019857,2922762,2922825,
 2923091,5020249,2923154,2923217,2923609,11080268,13177426,2691699,11080331,13177489,
 2691762,11080394,2691825,11080457,9216588,11313746,8983628,9216651,11313809,8983691,
 11080849,8983754,9216777,8983817,7119948,9217106,6886988,7120011,9217169,6887051,
 8984209,6887114,7120137,6887177,5023308,7120466,4790348,5023371,7120529,4790411,
 6887569,4790474,5023497,4790537,2926668,5023826,2693708,2926731,5023889,2693771,
 4790929,2693834,2926857,2693897,2927186,2694226,2927249,2694289,12948043,2462316,
 4559474,12948106,2462379,4559537,12948169,2462442,2462505,11084363,13181521,2695794,
 10851403,12948561,2462834,2695857,10851466,11084489,2462897,10851529,9220683,11317841,
 8987723,9220746,8754763,10851921,9220809,8754826,8987849,8754889,7124043,9221201,
 6891083,7124106,6658123,6891146,7124169,6658186,6891209,6658249,5027403,7124561,
 4794443,5027466,4561483,4794506,5027529,4561546,4794569,4561609,2930763,5027921,
 2697803,2930826,2464843,2697866,2930889,2464906,2697929,2464969,2931281,2698321,
 12952138,2466411,4563569,12952201,2466474,2466537,11088458,2699889,10855498,11088521,
 2466929,10855561,9224778,8991818,9224841,8758858,8991881,8758921,7128138,6895178,
 7128201,6662218,6895241,6662281,5031498,4798538,5031561,4565578,4798601,4565641,
 2934858,2701898,2934921,2468938,2701961,2469001,12956233,2470506,2470569,11092553,
 10859593,9228873,8995913,8762953,7132233,6899273,6666313,5035593,4802633,4569673,
 2938953,2705993,2473033,2474601,9474636,11571794,9474699,11571857,9474762,9474825,
 7377996,9475154,7378059,9475217,7378122,7378185,5281356,7378514,5281419,7378577,
 5281482,5281545,3184716,5281874,3184779,5281937,3184842,3184905,3185234,3185297,
 11342411,13439569,2953842,11342474,2953905,11342537,9478731,11575889,9245771,9478794,
 9245834,9478857,9245897,7382091,9479249,7149131,7382154,7149194,7382217,7149257,
 5285451,7382609,5052491,5285514,5052554,5285577,5052617,3188811,5285969,2955851,
 3188874,2955914,3188937,2955977,3189329,2956369,13210186,2724459,4821617,13210249,
 2724522,2724585,11346506,2957937,11113546,11346569,2724977,11113609,9482826,9249866,
 9482889,9016906,9249929,9016969,7386186,7153226,7386249,6920266,7153289,6920329,
 5289546,5056586,5289609,4823626,5056649,4823689,3192906,2959946,3192969,2726986,
 2960009,2727049,2495076,4592234,2495139,4592297,2495202,2495265,13214281,2728554,
 12981321,2495594,2728617,2495657,11350601,11117641,10884681,9486921,9253961,9021001,
 8788041,7390281,7157321,6924361,6691401,5293641,5060681,4827721,4594761,3197001,
 2964041,2731081,2499171,4596329,2499234,2499297,2732649,2499689,2503266,2503329,
 2507361,9736779,9736842,9736905,7640139,9737297,7640202,7640265,5543499,7640657,
 5543562,5543625,3446859,5544017,3446922,3446985,3447377,11604554,3215985,11604617,
 9740874,9507914,9740937,9507977,7644234,7411274,7644297,7411337,5547594,5314634,
 5547657,5314697,3450954,3217994,3451017,3218057,13472329,2986602,2986665,11608649,
 11375689,9744969,9512009,9279049,7648329,7415369,7182409,5551689,5318729,5085769,
 3455049,3222089,2989129,2757219,4854377,2757282,2757345,2990697,2757737,2527836,
 4624994,2527899,4625057,2527962,2528025,2761314,2528354,2761377,2528417,2531931,
 4629089,2531994,2532057,2765409,2532449,2536026,2536089,2540121,7902282,7902345,
 5805642,5805705,3709002,3709065,9770057,7906377,7673417,5809737,5576777,3713097,
 3480137,3248745,3019362,3019425,2789979,4887137,2790042,2790105,3023457,2790497,
 2560596,4657754,2560659,4657817,2560722,2560785,2794074,2561114,2794137,2561177,
 2564691,4661849,2564754,2564817,2798169,2565209,2568786,2568849,3281505,3052122,
 3052185,2822739,4919897,2822802,2822865,3056217,2823257,2826834,2826897},{
 4493933,6591091,2396838,4493996,6591154,2396901,4494059,6591217,2396964,4494122,
 2397027,12883020,2397293,4494451,12883083,2397356,4494514,12883146,2397419,4494577,
 12883209,2397482,10786380,12883538,2397811,10786443,12883601,2397874,10786506,2397937,
 10786569,8689740,10786898,8689803,10786961,8689866,8689929,6593100,8690258,6593163,
 8690321,6593226,6593289,4496460,6593618,4496523,6593681,4496586,2400870,4498028,
 6595186,2400933,4498091,6595249,2400996,4498154,2401059,4498217,2401122,12887115,
 2401388,4498546,12887178,2401451,4498609,12887241,2401514,2401577,10790475,12887633,
 2401906,10790538,2401969,10790601,8693835,10790993,8693898,8693961,6597195,8694353,
 6597258,6597321,4500555,6597713,4500618,4500681,2404965,4502123,6599281,2405028,
 4502186,2405091,4502249,2405154,2405217,12891210,2405483,4502641,12891273,2405546,
 2405609,10794570,2406001,10794633,8697930,8697993,6601290,6601353,4504650,4504713,
 2409060,4506218,2409123,4506281,2409186,2409249,12895305,2409578,2409641,10798665,
 8702025,6605385,2413155,4510313,2413218,2413281,2413673,2417250,2417313,2658918,
 4756076,6853234,2658981,4756139,6853297,2659044,4756202,2659107,4756265,2659170,
 13145163,2659436,4756594,13145226,2659499,4756657,13145289,2659562,2659625,11048523,
 13145681,2659954,11048586,2660017,11048649,8951883,11049041,8951946,8952009,6855243,
 8952401,6855306,6855369,4758603,6855761,4758666,4758729,4526693,6623851,8721009,
 2429598,4526756,6623914,2429661,4526819,6623977,2429724,4526882,2429787,4526945,
 2429850,2663013,4760171,6857329,2430053,2663076,4760234,2430116,4527274,4760297,
 2430179,2663202,2430242,2663265,2430305,13149258,2663531,4760689,12916298,13149321,
 2430571,2663594,12916361,2430634,2663657,2430697,11052618,2664049,10819658,11052681,
 2431089,10819721,8955978,8723018,8956041,8723081,6859338,6626378,6859401,6626441,
 4762698,4762761,2433630,4530788,6627946,2433693,4530851,6628009,2433756,4530914,
 2433819,4530977,2433882,2433945,2667108,4764266,2434148,2667171,4764329,2434211,
 4531369,2434274,2667297,2434337,13153353,2667626,12920393,2434666,2667689,2434729,
 11056713,10823753,8960073,8727113,6863433,6630473,4766793,2437725,4534883,6632041,
 2437788,4534946,2437851,4535009,2437914,2437977,2671203,4768361,2438243,2671266,
 2438306,2671329,2438369,2671721,2438761,2441820,4538978,2441883,4539041,2441946,
 2442009,2675298,2442338,2675361,2442401,2445915,4543073,2445978,2446041,2679393,
 2446433,2450010,2450073,2921061,5018219,7115377,2921124,5018282,2921187,5018345,
 2921250,2921313,13407306,2921579,5018737,13407369,2921642,2921705,11310666,2922097,
 11310729,9214026,9214089,7117386,7117449,5020746,5020809,2691678,4788836,6885994,
 2691741,4788899,6886057,2691804,4788962,2691867,4789025,2691930,2691993,2925156,
 5022314,2692196,2925219,5022377,2692259,4789417,2692322,2925345,2692385,13411401,
 2925674,13178441,2692714,2925737,2692777,11314761,11081801,9218121,8985161,7121481,
 6888521,5024841,4559453,6656611,8753769,2462358,4559516,6656674,2462421,4559579,
 6656737,2462484,4559642,2462547,4559705,2462610,2695773,4792931,6890089,2462813,
 2695836,4792994,2462876,4560034,4793057,2462939,2695962,2463002,2696025,2463065,
 2929251,5026409,2696291,2929314,2463331,2696354,2929377,2463394,2696417,2463457,
 2929769,2696809,2463849,2466390,4563548,6660706,2466453,4563611,6660769,2466516,
 4563674,2466579,4563737,2466642,2466705,2699868,4797026,2466908,2699931,4797089,
 2466971,4564129,2467034,2700057,2467097,2933346,2700386,2933409,2467426,2700449,
 2467489,2470485,4567643,6664801,2470548,4567706,2470611,4567769,2470674,2470737,
 2703963,4801121,2471003,2704026,2471066,2704089,2471129,2937441,2704481,2471521,
 2474580,4571738,2474643,4571801,2474706,2474769,2708058,2475098,2708121,2475161,
 2478675,4575833,2478738,2478801,2712153,2479193,2482770,2482833,3183204,5280362,
 3183267,5280425,3183330,3183393,3183722,3183785,11572809,9476169,7379529,5282889,
 2953821,5050979,7148137,2953884,5051042,2953947,5051105,2954010,2954073,3187299,
 5284457,2954339,3187362,2954402,3187425,2954465,3187817,2954857,2724438,4821596,
 6918754,2724501,4821659,6918817,2724564,4821722,2724627,4821785,2724690,2724753,
 2957916,5055074,2724956,2957979,5055137,2725019,2958042,2725082,2958105,2725145,
 3191394,2958434,3191457,2725474,2958497,2725537,4592213,6689371,8786529,2495118,
 4592276,6689434,2495181,4592339,6689497,2495244,4592402,2495307,4592465,2495370,
 2728533,4825691,6922849,2495573,2728596,4825754,2495636,4592794,4825817,2495699,
 2728722,2495762,2728785,2495825,2962011,5059169,2729051,2962074,2496091,2729114,
 2962137,2496154,2729177,2496217,3195489,2962529,2729569,2496609,2499150,4596308,
 6693466,2499213,4596371,6693529,2499276,4596434,2499339,4596497,2499402,2499465,
 2732628,4829786,2499668,2732691,4829849,2499731,4596889,2499794,2732817,2499857,
 2966106,2733146,2966169,2500186,2733209,2500249,2503245,4600403,6697561,2503308,
 4600466,2503371,4600529,2503434,2503497,2736723,4833881,2503763,2736786,2503826,
 2736849,2503889,2970201,2737241,2504281,2507340,4604498,2507403,4604561,2507466,
 2507529,2740818,2507858,2740881,2507921,2511435,4608593,2511498,2511561,2744913,
 2511953,3445347,5542505,3445410,3445473,3445865,3215964,5313122,3216027,5313185,
 3216090,3216153,3449442,3216482,3449505,3216545,2986581,5083739,7180897,2986644,
 5083802,2986707,5083865,2986770,2986833,3220059,5317217,2987099,3220122,2987162,
 3220185,2987225,3453537,3220577,2987617,2757198,4854356,6951514,2757261,4854419,
 6951577,2757324,4854482,2757387,4854545,2757450,2757513,2990676,5087834,2757716,
 2990739,5087897,2757779,4854937,2757842,2990865,2757905,3224154,2991194,3224217,
 2758234,2991257,2758297,4624973,6722131,8819289,4625036,6722194,4625099,6722257,
 4625162,4625225,2761293,4858451,6955609,2528333,2761356,4858514,2528396,4625554,
 4858577,2528459,2761482,2528522,2761545,2528585,2994771,5091929,2761811,2994834,
 2528851,2761874,2994897,2528914,2761937,2528977,3228249,2995289,2762329,2529369,
 4629068,6726226,4629131,6726289,4629194,4629257,2765388,4862546,2532428,2765451,
 4862609,2532491,4629649,2532554,2765577,2532617,2998866,2765906,2998929,2532946,
 2765969,2533009,4633163,6730321,4633226,4633289,2769483,4866641,2536523,2769546,
 2536586,2769609,2536649,3002961,2770001,2537041,4637258,4637321,2773578,2540618,
 2773641,2540681,3707490,3707553,3478107,5575265,3478170,3478233,3711585,3478625,
 3248724,5345882,3248787,5345945,3248850,3248913,3482202,3249242,3482265,3249305,
 3019341,5116499,7213657,3019404,5116562,3019467,5116625,3019530,3019593,3252819,
 5349977,3019859,5117017,3019922,3252945,3019985,3486297,3253337,3020377,4887116,
 6984274,4887179,6984337,4887242,4887305,3023436,5120594,2790476,4887634,5120657,
 2790539,3023562,2790602,3023625,2790665,3256914,3023954,3256977,2790994,3024017,
 2791057,6754891,8852049,6754954,6755017,4891211,6988369,4658251,4891274,4658314,
 4891337,4658377,3027531,5124689,2794571,3027594,2561611,2794634,3027657,2561674,
 2794697,2561737,3261009,3028049,2795089,2562129,6758986,6759049,4895306,4662346,
 4895369,4662409,3031626,2798666,3031689,2565706,2798729,2565769,6763081,4899401,
 4666441,3035721,2802761,2569801,3740250,3740313,3510867,5608025,3510930,3510993,
 3744345,3511385,3281484,5378642,3281547,5378705,3281610,3281673,3514962,3282002,
 3515025,3282065,5149259,7246417,5149322,5149385,3285579,5382737,3052619,5149777,
 3052682,3285705,3052745,3519057,3286097,3053137,7017034,7017097,5153354,4920394,
 5153417,4920457,3289674,3056714,3289737,2823754,3056777,2823817,7021129,5157449,
 4924489,3293769,3060809,2827849,3773010,3773073,3543627,5640785,3543690,3543753,
 3777105,3544145,5411402,5411465,3547722,3314762,3547785,3314825,7279177}};

const uint32_t barRap[2][52] = {{
 74441,103113,103561,74889,71305,71753,75337,104009,107593,136265,
 139849,111177,82505,78921,78473,107145,135817,135761,135754,107082,
 103498,103050,103057,103001,102994,102987,74315,74322,74329,74385,
 74833,103505,107089,78417,78410,74826,71242,70794,70801,70745,
 70738,70731,70283,70227,70234,70241,70297,70290,70346,70353,
 70409,70857,},{
 38041,41625,42073,45657,45713,46161,49745,49801,50249,46665,
 46217,45769,42185,42633,43081,39497,39049,38993,42577,42570,
 42122,42129,41681,41737,38153,38601,38545,38538,38482,42066,
 45650,45643,42059,38475,38027,38034,38090,38097,37649,37593,
 37586,37530,37523,37467,37460,37516,37964,41548,41555,41562,
 37978,37985,}};


