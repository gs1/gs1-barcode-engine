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

#define PRNT 0 // prints symbol data if 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "release.h"

#define	TRUE	1
#define	FALSE	0

#define UCHAR unsigned char
#define UINT	unsigned short
#define ULONG	unsigned long

#define MAX_FNAME 120
#define MAX_DATA (75+2361)

#define CCB2_WIDTH 57 // 2 column cca/b
#define CCB2_ELMNTS 31 // includes qz's

#define CCA3_WIDTH 74 // 3 column cca
#define CCA3_ELMNTS 39 // includes qz's
#define MAX_CCA3_ROWS 8	// cca-3 max rows

#define CCB3_WIDTH 84 // 3 column ccb
#define CCB3_ELMNTS 45 // includes qz's

#define CCB4_WIDTH 101 // 4 column cca/b
#define CCB4_ELMNTS 53 // includes qz's
#define MAX_CCB4_ROWS 44	// ccb-4 max rows

enum {
	sNONE = 0,	    //none defined
	sRSS14,			//RSS-14
	sRSS14T,		//RSS-14 Truncated
	sRSS14S,		//RSS-14 Stacked
	sRSS14SO,		//RSS-14 Stacked Omnidirectional
	sRSSLIM,		//RSS Limited
	sRSSEXP,		//RSS Expanded
	sUPCA,			//UPC-A
	sUPCE,			//UPC-E
	sEAN13,			//EAN-13
	sEAN8,			//EAN-8
	sUCC128_CCA,	//UCC/EAN-128 with CC-A or CC-B
	sUCC128_CCC,	//UCC/EAN-128 with CC-C
	sNUMSYMS,       //Number of symbologies
};

struct sParams {
	int sym;				// symbology type
	int inputFlag;			// 1 = dataStr, 2 = dataFile
	int pixMult;				// pixels per X
	int Xundercut;				// X pixels to undercut
	int Yundercut;			// Y pixels to undercut
	int sepHt;					// separator row height
	int segWidth;
	int bmp;						// TRUE is BMP else TIF file output
	int linHeight;				// height of UCC/EAN-128 in X
	FILE *outfp;
	char dataFile[MAX_FNAME+1];
	char outFile[MAX_FNAME+1];
	char dataStr[MAX_DATA+1];
};


struct sPrints {
	int elmCnt;
	int leftPad;
	int rightPad;
	int guards;
	int height;
	int whtFirst;
	int reverse;
	UCHAR *pattern;
};

// subroutine prototypes:
void bmpHeader(long xdim, long ydim, FILE *oFile);
void tifHeader(long xdim, long ydim, FILE *oFile);
void printElmnts(struct sParams *params, struct sPrints *prints);
struct sPrints *cnvSeparator(struct sParams *params, struct sPrints *prints);
int CC2enc(UCHAR str[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CC3enc(UCHAR str[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CC4enc(UCHAR str[], UCHAR pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CCCenc(UCHAR str[], UCHAR pattern[]);
void init928(void);
void initLogTables(void);
int *getRSSwidths(int val, int n, int elements, int maxWidth, int noNarrow);
int pack(UCHAR str[], UCHAR bitField[] );
int check2DData(UCHAR dataStr[]);
void putBits(UCHAR bitField[], int bitPos, int length, UINT bits);
void RSS14(struct sParams *params);
void RSS14S(struct sParams *params);
void RSS14SO(struct sParams *params);
void RSSLim(struct sParams *params);
void RSSExp(struct sParams *params);
void EAN13(struct sParams *params);
void EAN8(struct sParams *params);
void UPCE(struct sParams *params);
void U128A(struct sParams *params);
void U128C(struct sParams *params);
