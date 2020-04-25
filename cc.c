/***************************************************************************
*
* File: 	CC.C
*
* Author: 
*
* Date: 	2000-03-30
*
* Description: 2D Composite Component routines.
*
* History:
*		010828 Fixed doLimMethods() bug for data (01)9...(310x/320x)...
****************************************************************************/
#include "rssenc.h"
#include "bardata.h"
#include "barrap.h"

#define MAX_CCA2_SIZE 6 // index to 167 in CC2Sizes
#define MAX_CCA3_SIZE 4 // index to 167 in CC3Sizes
#define MAX_CCA4_SIZE 4 // index to 197 in CC4Sizes

#define MAX_CW 176 // ccb-4 max codewords
#define MAX_BYTES 148 // maximum byte mode capacity for ccb4

#define MAX_CCC_CW 863	// ccc max data codewords
#define MAX_CCC_ROWS 90	// ccc max rows
#define MAX_CCC_BYTES 1033 // maximum byte mode capacity for ccc

#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

struct encodeT {
	UCHAR *str;
	int iStr;
	UCHAR *bitField;
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
extern UCHAR ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

// CC-C external variables
extern int colCnt; // after set in main, may be decreased by getUnusedBitCnt
extern int rowCnt; // determined by getUnusedBitCnt
extern int eccCnt; // determined by getUnusedBitCnt

void encCCA2(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void encCCB2(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void encCCA3(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void encCCB3(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void encCCA4(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void encCCB4(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void encCCC(int byteCnt, UCHAR bitField[], UINT codeWords[],
							UCHAR patCCC[]);
void imgCCA2(int size, UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void imgCCB2(int size, UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void imgCCA3(int size, UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void imgCCB3(int size, UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void imgCCA4(int size, UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void imgCCB4(int size, UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
void imgCCC(UINT codeWords[], UCHAR patCCC[]);

int procNUM(struct encodeT *encode);
int procALNU(struct encodeT *encode);
int procISO(struct encodeT *encode);
int procALPH(struct encodeT *encode);
int doMethods(struct encodeT *encode);
int insertPad(struct encodeT *encode);
int getUnusedBitCnt(int iBit, int *size);
int testAI90(struct encodeT *encode);
void procAI90(struct encodeT *encode);
void encodeAI90(struct encodeT *encode);
void nextAI(struct encodeT *encode);
int encode928(UCHAR bitString[], UINT codeWords[], int bitLng);
void encode900(UCHAR byteArr[], UINT codeWords[], int byteLng);
int getBit(UCHAR bitStr[], int bitPos);

void genECC(int dsize, int csize, UINT sym[]);
void genPoly(int eccSize);
void genLog(void);
int gfMul(int p1, int p2);

int doLinMethods(UCHAR str[], int *iStr, UCHAR bitField[], int *iBit);
UINT yymmdd(UCHAR str[]);
void cnv13 (UCHAR str[], int *iStr, UCHAR bitField[], int *iBit);
void cnv12 (UCHAR str[], int *iStr, UCHAR bitField[], int *iBit);

static int *CCSizes; // will point to CCxSizes

static int CC2Sizes[] = {	59,78,88,108,118,138,167, // cca sizes
												208,256,296,336,			// ccb sizes
												0 };

int CC2enc(UCHAR str[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS] ) {

static int rows[11] = { 5,6,7,8,9,10,12,  17,20,23,26 }; // 7 CCA & 4 CCB row counts

UCHAR bitField[MAX_BYTES];
UINT codeWords[MAX_CW];
int size;
int i;

	linFlag = 0;
	CCSizes = CC2Sizes;
	if ((i=check2DData(str)) != 0) {
		printf("\nillegal character in 2D data = '%c'", str[i]);
		errFlag = TRUE;
		return(0);
	}
#if PRNT
	printf("%s\n", str);
#endif
	size = pack(str, bitField);
	if (size < 0 || CC2Sizes[size] == 0) {
		printf("\ndata error");
		errFlag = TRUE;
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

int CC3Sizes[] = {	78,98,118,138,167, // cca sizes
												208,304,416,536,648,768,		// ccb sizes
												0 };

int CC3enc(UCHAR str[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS] ) {

static int rows[11] = { 4,5,6,7,8,  15,20,26,32,38,44 }; // 5 CCA & 6 CCB row counts

UCHAR bitField[MAX_BYTES];
UINT codeWords[MAX_CW];
int size;
int i;

	linFlag = 0;
	CCSizes = CC3Sizes;
	if ((i=check2DData(str)) != 0) {
		printf("\nillegal character in 2D data = '%c'", str[i]);
		errFlag = TRUE;
		return(0);
	}
#if PRNT
	printf("%s\n", str);
#endif
	size = pack(str, bitField);
	if (size < 0 || CC3Sizes[size] == 0) {
		printf("\ndata error");
		errFlag = TRUE;
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

int CC4enc(UCHAR str[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS] ) {

static int rows[13] = { 3,4,5,6,7,  10,12,15,20,26,32,38,44 }; // 5 CCA & 8 CCB row counts

UCHAR bitField[MAX_BYTES];
UINT codeWords[MAX_CW];
int size;
int i;

	linFlag = 0;
	CCSizes = CC4Sizes;
	if ((i=check2DData(str)) != 0) {
		printf("\nillegal character in 2D data = '%c'", str[i]);
		errFlag = TRUE;
		return(0);
	}
#if PRNT
	printf("%s\n", str);
#endif
	size = pack(str, bitField);
	if (size < 0 || CC4Sizes[size] == 0) {
		printf("\ndata error");
		errFlag = TRUE;
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

int CCCenc(UCHAR str[], UCHAR patCCC[] ) {

UCHAR bitField[MAX_CCC_BYTES];
UINT codeWords[MAX_CCC_CW];
int byteCnt;
int i;

	linFlag = -1; // CC-C flag value
	if ((i=check2DData(str)) != 0) {
		printf("illegal character '%c'\n", str[i]);
		return(FALSE);
	}
	printf("%s\n", str);
	if((byteCnt = pack(str, bitField)) < 0) {
		printf("data error\n");
		return(FALSE);
	}
	encCCC(byteCnt, bitField, codeWords, patCCC);
	return(TRUE);
}


void encCCA2(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataCw[7] = { 6,8,9,11,12,14,17 };
static int eccCw[7] = { 4,4,5,5,6,6,7 };

	encode928(bitField, codeWords, CC2Sizes[size]);

	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCA2(size, codeWords, pattern);
	return;
}

void encCCB2(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

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

void encCCA3(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataCw[5] = { 8,10,12,14,17 };
static int eccCw[5] = { 4,5,6,7,7 };

	encode928(bitField, codeWords, CC3Sizes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCA3(size, codeWords, pattern);
	return;
}

void encCCB3(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

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

void encCCA4(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int dataCw[5] = { 8,11,14,17,20 };
static int eccCw[5] = { 4,5,6,7,8 };

	encode928(bitField, codeWords, CC4Sizes[size]);
	genECC(dataCw[size], eccCw[size], codeWords);
	imgCCA4(size, codeWords, pattern);
	return; 
}

void encCCB4(int size, UCHAR bitField[], UINT codeWords[],
							UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

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

void encCCC(int byteCnt, UCHAR bitField[], UINT codeWords[], UCHAR patCCC[]) {

int nonEccCwCnt;

	nonEccCwCnt = colCnt*rowCnt-eccCnt;
	codeWords[0] = nonEccCwCnt;
	codeWords[1] = 920; // insert UCC/EAN flag and byte mode latch
	codeWords[2] = 
		(byteCnt % 6 == 0) ? 924 : 901; // 924 iff even multiple of 6
	encode900(bitField, &codeWords[3], byteCnt);
	genECC(nonEccCwCnt, eccCnt, codeWords);
	imgCCC(codeWords, patCCC);
	return; 
}
                    


void imgCCA2(int size, UINT codeWords[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[7] = { 5,6,7,8,9,10,12 };
static int raps[7] = { 39,1,32,8,14,43,20 };
    
ULONG bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index           
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*2]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*2+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+32)%52]; // right rap (rotation 32)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

void imgCCB2(int size, UINT codeWords[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[4] = { 17,20,23,26 };
static int raps[4] = { 36,19,9,27 };
static int rotate[4] = { 0,0,8,8 };
    
ULONG bars;
int rowCnt, rapL;
int i, j;
           
	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*2]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*2+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {                                       
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[0][(rapL+rotate[size])%52]; // right rap (rotation 0 or 8)
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		pattern[i][1+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+1] = 1; // qz       
		rapL = (rapL+1)%52;
  }
	return;
}

void imgCCA3(int size, UINT codeWords[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[5] = { 4,5,6,7,8 };
static int raps[5] = { 11,1,5,15,21 };
    
ULONG bars;
int rowCnt, rapL;
int i, j;
           
	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index           
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barData[rapL%3][codeWords[i*3]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {                                       
			// get 8 3-bit widths left to right:
			pattern[i][1  +j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[1][(rapL+32)%52]; // center rap (rotation 32)
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1  +8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*3+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {                                       
			// get 8 3-bit widths left to right:
			pattern[i][1  +8+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*3+2]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {                                       
			// get 8 3-bit widths left to right:
			pattern[i][1  +8+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[0][(rapL+32+32)%52]; // right rap (rotation 64)
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1  +8+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		pattern[i][1  +8+6+8+8+6] = 1; // right guard
		pattern[i][1  +8+6+8+8+6+1] = 1; // qz       
		rapL = (rapL+1)%52;
	}
	return;
}

void imgCCB3(int size, UINT codeWords[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[6] = { 15,20,26,32,38,44 };
static int raps[6] = { 37,1,1,21,15,1 };
static int rotate[6] = { 0,16,8,8,16,24 };
    
ULONG bars;
int rowCnt, rapL;
int i, j;
           
	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index           
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*3]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {                                       
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[1][(rapL+rotate[size])%52]; // center rap (rotation 0,8,16, or 24)
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*3+1]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {                                       
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*3+2]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[0][(rapL+rotate[size]*2)%52]; // right rap (double rotation)
		for (j = 0; j < 6; j++) {                                       
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		pattern[i][1+6+8+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+6+8+8+6+1] = 1; // qz       
		rapL = (rapL+1)%52;
  }
	return;
}

void imgCCA4(int size, UINT codeWords[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[5] = { 3,4,5,6,7 };
static int raps[5] = { 40,43,46,34,29 };

ULONG bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+1]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[1][(rapL+32)%52]; // center rap (rotation 32)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*4+2]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+3]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[0][(rapL+32+32)%52]; // right rap (double rotation)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		pattern[i][1+6+8+8+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

void imgCCB4(int size, UINT codeWords[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]) {

static int rows[8] = { 10,12,15,20,26,32,38,44 };
static int raps[8] = { 15,25,37,1,1,21,15,1 };
static int rotate[8] = { 0,0,0,16,8,8,16,24 };
    
ULONG bars;
int rowCnt, rapL;
int i, j;

	rowCnt = rows[size];
	rapL = raps[size]-1; // -1 to map to 0-51 array index
	for (i = 0; i < rowCnt; i++) {
		pattern[i][0] = 1; // qz
		bars = barRap[0][rapL]; // left rap
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*4]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barData[rapL%3][codeWords[i*4+1]]; // data1 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}		
		bars = barRap[1][(rapL+rotate[size])%52]; // center rap (rotation 0,8,16, or 24)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+2]]; // data2 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}
		bars = barData[rapL%3][codeWords[i*4+3]]; // data3 in row's cluster
		for (j = 0; j < 8; j++) {
			// get 8 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+j] = (UCHAR)(bars >> ((7-j)*3)) & 7;
		}
		bars = barRap[0][(rapL+rotate[size]*2)%52]; // right rap (double rotation)
		for (j = 0; j < 6; j++) {
			// get 6 3-bit widths left to right:
			pattern[i][1+6+8+8+6+8+8+j] = (UCHAR)(bars >> ((5-j)*3)) & 7;
		}
		pattern[i][1+6+8+8+6+8+8+6] = 1; // right guard
		pattern[i][1+6+8+8+6+8+8+6+1] = 1; // qz
		rapL = (rapL+1)%52;
	}
	return;
}

void imgCCC(UINT codeWords[], UCHAR patCCC[]) {

static UCHAR leftPtn[9] = { 2,8,1,1,1,1,1,1,3 }; // qz + start    
static UCHAR rightPtn[10] = { 7,1,1,3,1,1,1,2,1,2 }; // stop + qz    

int leftRowBase[3]; // right row is (left index+2) mod 3

ULONG bars;
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
			patCCC[row*((colCnt+4)*8+3) + bar+offset] = (UCHAR)(bars >> ((7-bar)*3)) & 7;
		}
		offset += bar;
		for (col = 0; col < colCnt; col++) {
			bars = barData[cluster][codeWords[cwNdx++]]; // codeword
			for (bar = 0; bar < 8; bar++) {                                       
				// get 8 3-bit widths left to right:
				patCCC[row*((colCnt+4)*8+3) + bar+offset] = (UCHAR)(bars >> ((7-bar)*3)) & 7;
			}
			offset += bar;
		}		
		// right row indicator
		bars = barData[cluster][rowFactor + leftRowBase[(cluster+2)%3]]; // right R.I.
		for (bar = 0; bar < 8; bar++) {                                       
			// get 8 3-bit widths left to right:
			patCCC[row*((colCnt+4)*8+3) + bar+offset] = (UCHAR)(bars >> ((7-bar)*3)) & 7;
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

int pack(UCHAR str[], UCHAR bitField[] ) {

struct encodeT encode;

int i;

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
			errFlag = TRUE;
			return(-1);
		} } /* end of case */
	}
  if (linFlag == -1) { // CC-C
		if (!insertPad(&encode)) { // will return FALSE if error
			printf("symbol too big\n");
			return(-1);
		}
		return(encode.iBit/8); // no error, return number of data bytes
	}
	else { // CC-A/B or RSS Exp
		return(insertPad(&encode));
	}
}

UCHAR iswhat[256] = { /* byte look up table with IS_XXX bits */
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

int check2DData(UCHAR dataStr[]) {
	int i;

	for (i = 0; iswhat[dataStr[i]] != 0x80; i++) {
		if (iswhat[dataStr[i]] == 0) {
			return(i); // error, unsupported character
		}
	}
	return(0);
}

int procNUM(struct encodeT *encode) {

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
			putBits(encode->bitField, encode->iBit, 4, (UINT)(char1+1 -(int)'0'));
			bitCnt -= 4;
			if (bitCnt > 0) {
				// 0 or 00 final pad
				putBits(encode->bitField, (encode->iBit)+4, bitCnt, 0);
			}
			encode->iBit += 4 + bitCnt;
		}
		else {
			// encode as digit & FNC1
			putBits(encode->bitField, encode->iBit, 7, (UINT)(((char1-(int)'0') * 11) + 10 + 8));
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
		putBits(encode->bitField, encode->iBit, 7, (UINT)((char1 * 11) + char2 + 8));
		encode->iBit += 7;
		return(NUM_MODE);
	}
}

int procALNU(struct encodeT *encode) {

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
		putBits(encode->bitField, encode->iBit, 5, (UINT)chr);
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
		putBits(encode->bitField, encode->iBit, 6, (UINT)(chr + 0x20));
		encode->iBit += 6;
	}
	return(encode->mode);
}

int procISO(struct encodeT *encode) {

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
		putBits(encode->bitField, encode->iBit, 5, (UINT)chr);
		encode->iBit += 5;
	}
	else if ((chr >= (int)'A') && (chr <= (int)'Z')) {
		// A-Z
		chr = chr - (int)'A' + 0x40;
		putBits(encode->bitField, encode->iBit, 7, (UINT)chr);
		encode->iBit += 7;
	}
	else if ((chr >= (int)'a') && (chr <= (int)'z')) {
		// a-z
		chr = chr - (int)'a' + 0x5A;
		putBits(encode->bitField, encode->iBit, 7, (UINT)chr);
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
		putBits(encode->bitField, encode->iBit, 8, (UINT)chr);
		encode->iBit += 8;
	}
	return(encode->mode);
}

int procALPH(struct encodeT *encode) {

int i;

	// check next char type
	if (isupper(encode->str[encode->iStr])) {
		// alpha
		putBits(encode->bitField, encode->iBit, 5, encode->str[encode->iStr]-65);
		encode->iBit += 5;
		encode->iStr += 1;
	}
	else if (isdigit(encode->str[encode->iStr])) {
		// number
		putBits(encode->bitField, encode->iBit, 6, encode->str[encode->iStr]+4);
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

int insertPad(struct encodeT *encode) {

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
		putBits(encode->bitField, encode->iBit, bitCnt, (UINT)chr);
		encode->iBit += bitCnt;
	}
	return(size);
}

int doMethods(struct encodeT *encode) {

UINT bits;
int iStr1;

	if (strlen(encode->str) >= 8 && encode->str[0] == '1' &&
				(encode->str[1] == '1' || encode->str[1] == '7')) {
		// method "10"
		putBits(encode->bitField, 0, 2, 2); // mfg/exp date-lot encodation method bit flag 10
		bits = ((UINT)(encode->str[2]-'0')*10 +
				(UINT)(encode->str[3]-'0')) * 384; // YY
		bits += ((UINT)(encode->str[4]-'0')*10 +
				(UINT)(encode->str[5]-'0') - 1) * 32; // MM
		bits += (UINT)(encode->str[6]-'0')*10 +
				(UINT)(encode->str[7]-'0'); // DD
		putBits(encode->bitField, 2, 16, bits); // date packed data
		putBits(encode->bitField, 2+16, 1,
				(UINT)((encode->str[1] == '1') ? 0 : 1)); // 0/1 bit for AI 11/17
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

int testAI90(struct encodeT *encode) {

UCHAR chr;

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
		encode->diNum = atoi(&encode->str[encode->iStr+2]);
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
		encode->diNum = atoi(&encode->str[encode->iStr+2]);
		encode->str[encode->iStr+5] = chr;
		encode->iStr += 6;
	}
	return(encode->diNum >= 0);
}

void procAI90(struct encodeT *encode) {

int i, j, k;
int alLessNu;
int diNum1, diAlpha1;
static UCHAR alphaTbl[] = "BDHIJKLNPQRSTVWZ"; // strlen must be 16

		// method "11", look ahead to find best compaction scheme
		j = 10000; // 10000: initial flag for non-numeric index
		alLessNu = 0; // upper-case - digit, < -9000 if non-alnu seen
		for (i = encode->iStr; i < strlen(encode->str); i++) {
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
			putBits(encode->bitField, encode->iBit, 2, encode->typeAI);
			encode->iBit += 2;
		}
		for (j = 0; j < 16; j++) {
			if (diAlpha1 == alphaTbl[j]) {
				break;
			}
		}
		if (diNum1 < 31 && j < 16) {
			putBits(encode->bitField, encode->iBit, 5, diNum1); // DI number < 31
			putBits(encode->bitField,
					encode->iBit+5, 4, j); // DI alpha from alphaTbl
			encode->iBit += 9;
		}
		else {
			putBits(encode->bitField, encode->iBit, 5, 31);
			putBits(encode->bitField,
					encode->iBit+5, 10, diNum1); // DI number >= 31
			putBits(encode->bitField,
					encode->iBit+15, 5, diAlpha1-65); // or alpha not in table
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

void encodeAI90(struct encodeT *encode) {

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
			errFlag = TRUE;
			return;
		} } /* end of case */
	}
	return;
}

void nextAI(struct encodeT *encode) {

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

int doLinMethods(UCHAR str[], int *iStr, UCHAR bitField[], int *iBit) {

UINT bits;
long weight;
char numStr[10];

	if (strlen(str) >= 26) {
		strncpy(numStr, &str[20], 6); // possible weight field
		numStr[6] = '\0';
	}
	// look for AI 01
	if (strlen(str)>=16 && str[0]=='0' && str[1]=='1') {

		// look for fixed length with AI 01[9] + 3103[0-32767]
		if (str[2]=='9' && (strlen(str)==16+10) &&
			str[16]=='3' && str[17]=='1' && str[18]=='0' && str[19]=='3' &&
			(weight=atol(numStr))<=32767L) {
			// method 0100, AI's 01 + 3103
			putBits(bitField, *iBit, 4, 4); // write method
			*iBit += 4;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 15, (UINT)weight); // write weight
			*iBit += 15;
			*iStr += 1+10; // skip check digit & jump weight field
		}

		// look for fixed length with AI 01[9] + 3202[0-009999]
		else if (str[2]=='9' && (strlen(str)==16+10) &&
			str[16]=='3' && str[17]=='2' && str[18]=='0' && str[19]=='2' &&
			(weight=atol(numStr))<=9999L) {
			// method 0101, AI's 01 + 3202
			putBits(bitField, *iBit, 4, 5); // write method
			*iBit += 4;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 15, (UINT)weight); // write weight
			*iBit += 15;
			*iStr += 1+10; // skip check digit & jump weight field
		}

		// look for fixed length with AI 01[9] + 3203[0-022767]
		else if (str[2]=='9' && (strlen(str)==16+10) &&
			str[16]=='3' && str[17]=='2' && str[18]=='0' && str[19]=='3' &&
			(weight=atol(numStr))<=22767L) {
			// method 0101, AI's 01 + 3203
			putBits(bitField, *iBit, 4, 5); // write method
			*iBit += 4;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 15, (UINT)weight+10000); // write weight
			*iBit += 15;
			*iStr += 1+10; // skip check digit & jump weight field
		}

		// look for AI 01[9] + 392[0-3]
		else if (str[2]=='9' && (strlen(str)>=16+4+1) &&
			str[16]=='3' && str[17]=='9' && str[18]=='2' &&
			(str[19]>='0' && str[19]<='3')) {
			// method 01100, AI's 01 + 392x + G.P.
			putBits(bitField, *iBit, 5+2, 0x0C<<2); // write method + 2 VLS bits
			*iBit += 5+2;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 2, (UINT)(str[19]-'0')); // write D.P.
			*iBit += 2;
			*iStr += 1+4; // skip check digit & jump price AI
		}

		// look for AI 01[9] + 393[0-3]
		else if (str[2]=='9' && (strlen(str)>=16+4+3+1) &&
			str[16]=='3' && str[17]=='9' && str[18]=='3' &&
			(str[19]>='0' && str[19]<='3')) {
			// method 01101, AI's 01 + 393x[NNN] + G.P.
			putBits(bitField, *iBit, 5+2, 0x0D<<2); // write method + 2 VLS bits
			*iBit += 5+2;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			putBits(bitField, *iBit, 2, (UINT)(str[19]-'0')); // write D.P.
			*iBit += 2;
			*iStr += 1+4; // skip check digit & jump price AI
			strncpy(numStr, &str[20], 3); // ISO country code
			numStr[3] = '\0';
			putBits(bitField, *iBit, 10, atoi(numStr)); // write ISO c.c.
			*iBit += 10;
			*iStr += 3; // jump ISO country code
		}

		// look for fixed length with AI 01[9] + 310x/320x[0-099999]
		else if (str[2]=='9' && (strlen(str)==16+10) &&
			str[16]=='3' && (str[17]=='1' || str[17]=='2') && str[18]=='0' &&
			(weight=atol(numStr))<=99999L) {
			// methods 0111000-0111001, AI's 01 + 3x0x no date
			bits = 0x38+(str[17]-'1');
			putBits(bitField, *iBit, 7, bits); // write method
			*iBit += 7;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			weight = weight + ((long)(str[19] - '0') * 100000L); // decimal digit
			putBits(bitField, *iBit, 4, (UINT)(weight>>16)); // write weight
			putBits(bitField, *iBit+4, 16, (UINT)(weight&0xFFFF));
			*iBit += 20;
			*iStr += 1+10; // jump check digit and weight field
			putBits(bitField, *iBit, 16, (UINT)38400); // write no date
			*iBit += 16;
		}

		// look for fixed length + AI 01[9] + 310x/320x[0-099999] + 11/13/15/17
		else if (str[2]=='9' && strlen(str)==16+10+8 &&
			str[16]=='3' && (str[17]=='1' || str[17]=='2') && str[18]=='0' &&
			(weight=atol(numStr))<=99999L &&
			str[26]=='1' &&
			(str[27]=='1' || str[27]=='3' || str[27]=='5' || str[27]=='7')) {
			// methods 0111000-0111111, AI's 01 + 3x0x + 1x
			bits = 0x38+(str[27]-'1')+(str[17] - '1');
			putBits(bitField, *iBit, 7, bits); // write method
			*iBit += 7;
			*iStr += 3; // skip AI 01 and PI 9
			cnv12(str, iStr, bitField, iBit); // write PID-12
			weight = weight + ((long)(str[19] - '0') * 100000L); // decimal digit
			putBits(bitField, *iBit, 4, (UINT)(weight>>16)); // write weight
			putBits(bitField, *iBit+4, 16, (UINT)(weight&0xFFFF));
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

UINT yymmdd(UCHAR str[]) {

UINT val;

	val = ((UINT)(str[0]-'0')*10 +
					(UINT)(str[1]-'0')) * 384; // YY
	val += ((UINT)(str[2]-'0')*10 +
					(UINT)(str[3]-'0') - 1) * 32; // MM
	val += (UINT)(str[4]-'0')*10 +
					(UINT)(str[5]-'0'); // DD
	return(val);
}

// converts 13 digits to 44 bits
void cnv13 (UCHAR str[], int *iStr, UCHAR bitField[], int *iBit) {

int i;

	putBits(bitField, *iBit, 4, (UINT)(str[*iStr] - '0')); // high order 4 bits
	*iBit += 4;
	*iStr += 1;
	for (i = 0; i < 4 ; i++) {
		putBits(bitField, *iBit, 10, (UINT)((UINT)(str[*iStr] - '0')*100 +
							(str[*iStr+1] - '0')*10 +
							str[*iStr+2] - '0')); // 10 bit groups bits
		*iBit += 10;
		*iStr += 3;
	}
	return;
}

// converts 12 digits to 40 bits
void cnv12 (UCHAR str[], int *iStr, UCHAR bitField[], int *iBit) {
int i;

	for (i = 0; i < 4 ; i++) {
		putBits(bitField, *iBit, 10, (UINT)((UINT)(str[*iStr] - '0')*100 +
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
int getUnusedBitCnt(int iBit, int *size) {

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
		*size = FALSE; // size used as error flag for CC-C
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
		*size = TRUE;
		return(byteCnt*8 - iBit);
	}
	return(-1);
}

void putBits(UCHAR bitField[], int bitPos, int length, UINT bits) {
	int i, maxBytes;

	if (linFlag == -1) {
		maxBytes = MAX_CCC_BYTES; // CC-C
	}
	else {
		maxBytes = MAX_BYTES; // others
	}
	if ((bitPos+length > maxBytes*8) || (length > 16)) {
		printf("\nputBits error, %d, %d\n", bitPos, length);
		errFlag = TRUE;
		return;
	}
	for (i = length-1; i >= 0; i--) {
		if ((bits & 1) != 0) {
			bitField[(bitPos+i)/8] |= 0x80 >> ((bitPos+i)%8);
		}
		else {
			bitField[(bitPos+i)/8] &= ~(0x80 >> ((bitPos+i)%8));
		}
		bits >>= 1;
	}
	return;
}
                     
static UINT pwr928[69][7];
                    
/* initialize pwr928 encoding table */
void init928(void) {

int i, j, v;
int cw[7];

	cw[6] = 1L;
	for (i = 5; i >= 0; i--) {
		cw[i] = 0;
	}
	for (i = 0; i < 7; i++) pwr928[0][i] = cw[i];
	for (j = 1; j < 69; j++) {
		for (v = 0, i = 6; i >= 1; i--) {
			v = (2 * cw[i]) + (v / 928);
			pwr928[j][i] = cw[i] = v % 928;
		}
		pwr928[j][0] = cw[0] = (2 * cw[0]) + (v / 928);
	}
	return;
}

/* converts bit string to base 928 values, codeWords[0] is highest order */
int encode928(UCHAR bitString[], UINT codeWords[], int bitLng) {

int i, j, b, bitCnt, cwNdx, cwCnt, cwLng;

	for (cwNdx = cwLng = b = 0; b < bitLng; b += 69, cwNdx += 7) {
		bitCnt = min(bitLng-b, 69);
		cwLng += cwCnt = bitCnt/10 + 1;
		for (i = 0; i < cwCnt; i++) codeWords[cwNdx+i] = 0; /* init 0 */
		for (i = 0; i < bitCnt; i++) {
			if (getBit(bitString, b+bitCnt-i-1)) {
				for (j = 0; j < cwCnt; j++) {
					codeWords[cwNdx+j] += pwr928[i][j+7-cwCnt];
				}
			}
		}
		for (i = cwCnt-1; i > 0; i--) {
			/* add "carries" */
			codeWords[cwNdx+i-1] += codeWords[cwNdx+i]/928;
			codeWords[cwNdx+i] %= 928;
		}
	}
	return(cwLng);
}

/* converts bytes to base 900 values (codeWords), codeWords[0] is highest order */
void encode900(UCHAR byteArr[], UINT codeWords[], int byteLng) {
                                 
static UINT pwrByte[6][5] = { {0,0,0,0,1}, {0,0,0,0,256}, {0,0,0,72,736},
					{0,0,20,641,316}, {0,5,802,385,796}, {1,608,221,686,376}  };
                                 
int i, j, bCnt, cwNdx;
ULONG cw, t, carry, cwArr[5];

	for (cwNdx = bCnt = 0; bCnt < byteLng-5; cwNdx += 5, bCnt += 6) {
		// init cwArr to 6th byte
		for (i = 0; i < 4; i++) cwArr[i] = 0L;
		cwArr[4] = (ULONG)byteArr[bCnt + 5];
		for (i = 4; i >= 0; i--) {
	  	// add in 5th thru 1st bytes multilpied by pwrByte subarry
			cw = (ULONG)byteArr[bCnt + i];
			carry = 0L;
			for (j = 4; j >= 0; j--) {
				t = cwArr[j] + cw * (ULONG)pwrByte[5-i][j] + carry;
				carry = t / 900L; 
				cwArr[j] = t % 900L;
			}
		}
		// transfer 5 cwArr packed data to codeWords
		for (i = 0; i < 5; i++) {
			codeWords[cwNdx + i] = (UINT)cwArr[i];
		}
	}
	// transfer 5 or less remaining cwArr to codeWords as is
	for (i = 0; i < byteLng - bCnt; i++) {
		codeWords[cwNdx + i] = byteArr[bCnt + i];
	}
	return;
}



/* gets bit in bitString at bitPos */
int getBit(UCHAR bitStr[], int bitPos) {
		return(((bitStr[bitPos/8] & (0x80>>(bitPos%8))) == 0) ?	0 : 1);
}

/* GF(929) log and antilog tables: */
int gfPwr[928];
int gfLog[929];
int gpa[512];

void genECC(int dsize, int csize, UINT sym[]) {
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
			sym[dsize+i] = (sym[dsize+i+1] + 929 - gfMul(t, gpa[csize-1 - i])) % 929;
		}
		sym[dsize+csize-1] = (929 - gfMul(t, gpa[0])) % 929;
	}
	for (i = dsize; i < dsize+csize; i++) {
		sym[i] = (929 - sym[i]) % 929; 
	}
	return;
}

void genPoly(int eccSize) {
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

int gfMul(int a,int b) {

	if ((a == 0) || (b == 0)) return(0);
	return(gfPwr[(gfLog[a] + gfLog[b]) % 928]);
}

int gfDiv(int a,int b) {

	if (b == 0) {
		fprintf(stderr, "\ndivide by zero error in gfDiv.");
		errFlag = TRUE;
		return(0);
	}
	if (a == 0) return(0);
	return(gfPwr[(928 + gfLog[a] - gfLog[b]) % 928]);
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

