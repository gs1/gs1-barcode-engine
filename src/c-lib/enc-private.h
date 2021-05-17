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

#ifndef ENC_PRIVATE_H
#define ENC_PRIVATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "gs1encoders.h"


#define PRNT 0 // prints symbol data if 1


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


#include "cc.h"
#include "driver.h"
#include "ean.h"
#include "rss14.h"
#include "rssexp.h"
#include "rsslim.h"
#include "rssutil.h"
#include "ucc128.h"


struct gs1_encoder {

	// members with accessors
	int sym;		// symbology type
	bool fileInputFlag;	// true is dataFile else dataStr
	int pixMult;		// pixels per X
	int Xundercut;		// X pixels to undercut
	int Yundercut;		// Y pixels to undercut
	int sepHt;		// separator row height
	int segWidth;
	int format;		// BMP, TIF or RAW
	int linHeight;		// height of UCC/EAN-128 in X
	char dataStr[GS1_ENCODERS_MAX_DATA+1];
	char dataFile[GS1_ENCODERS_MAX_FNAME+1];
	char outFile[GS1_ENCODERS_MAX_FNAME+1];
	uint8_t *buffer;	// We may allocate an output buffer
	char VERSION[16];

	// per-instance globals
	FILE *outfp;
	size_t bufferCap;
	size_t bufferSize;
	int errFlag;
	char errMsg[512];
	int rowWidth;
	int line1;
	int linFlag;		// tells pack whether linear or cc is being encoded
	int colCnt;		// after set, may be decreased by getUnusedBitCnt
	int rowCnt;		// determined by getUnusedBitCnt
	int eccCnt;		// determined by getUnusedBitCnt
	uint8_t ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];
	const int *cc_CCSizes;	// will point to CCxSize
	int cc_gpa[512];
	uint8_t driver_line[MAX_LINE/8 + 1];
	uint8_t driver_lineUCut[MAX_LINE/8 + 1];
	struct sPrints *driver_rowBuffer;
	int driver_numRows;
	struct sPrints rss14_prntSep;
	uint8_t rss14_sepPattern[RSS14_SYM_W/2+2];
	struct sPrints rsslim_prntSep;
	uint8_t rsslim_sepPattern[RSSLIM_SYM_W];
	struct sPrints rssutil_prntSep;
	uint8_t rssutil_sepPattern[MAX_SEP_ELMNTS];
	int rss_util_widths[MAX_K];
	uint8_t ucc128_patCCC[UCC128_MAX_PAT];

};


#endif /* ENC_PRIVATE_H */
