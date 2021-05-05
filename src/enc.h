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

#ifndef ENC_H
#define ENC_H

#include <stdio.h>
#include <stdint.h>

#define PRNT 0 // prints symbol data if 1

#define MAX_FNAME 120
#define MAX_DATA (75+2361)

enum {
	sNONE = 0,	// none defined
	sRSS14,		// RSS-14
	sRSS14T,	// RSS-14 Truncated
	sRSS14S,	// RSS-14 Stacked
	sRSS14SO,	// RSS-14 Stacked Omnidirectional
	sRSSLIM,	// RSS Limited
	sRSSEXP,	// RSS Expanded
	sUPCA,		// UPC-A
	sUPCE,		// UPC-E
	sEAN13,		// EAN-13
	sEAN8,		// EAN-8
	sUCC128_CCA,	// UCC/EAN-128 with CC-A or CC-B
	sUCC128_CCC,	// UCC/EAN-128 with CC-C
	sNUMSYMS,	// Number of symbologies
};

struct sParams {
	int sym;		// symbology type
	int inputFlag;		// 1 = dataStr, 2 = dataFile
	int pixMult;		// pixels per X
	int Xundercut;		// X pixels to undercut
	int Yundercut;		// Y pixels to undercut
	int sepHt;		// separator row height
	int segWidth;
	int bmp;		// true is BMP else TIF file output
	int linHeight;		// height of UCC/EAN-128 in X
	FILE *outfp;
	char dataFile[MAX_FNAME+1];
	char outFile[MAX_FNAME+1];
	char dataStr[MAX_DATA+1];
	char *errMsg;
};


struct sPrints {
	int elmCnt;
	int leftPad;
	int rightPad;
	int guards;
	int height;
	int whtFirst;
	int reverse;
	uint8_t *pattern;
};


#include "rss14.h"
#include "rsslim.h"
#include "rssexp.h"
#include "ean.h"
#include "ucc128.h"


bool encode(struct sParams *params);

// globals
int errFlag;
char* errMsg;
int rowWidth;
int line1;
int linFlag; // tells pack whether linear or cc is being encoded

#endif /* ENC_H */
