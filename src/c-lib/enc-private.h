/**
 * GS1 barcode encoder library
 *
 * @author Copyright (c) 2021 GS1 AISBL.
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


// Implementation limits that can be changed
#define MAX_FNAME	120	// Maximum filename
#define MAX_DATA	8191	// Maximum input buffer size
#define MAX_PIXMULT	12	// Largest X dimension


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
#include "dm.h"
#include "driver.h"
#include "ean.h"
#include "mtx.h"
#include "qr.h"
#include "rss14.h"
#include "rssexp.h"
#include "rsslim.h"
#include "rssutil.h"
#include "ucc128.h"


struct gs1_encoder {

	// members with accessors
	int sym;		// symbology type
	int pixMult;		// pixels per X
	int Xundercut;		// X pixels to undercut
	int Yundercut;		// Y pixels to undercut
	int addCheckDigit;	// for EAN/UPC and RSS-14/Lim, calculated if true, otherwise validated
	int sepHt;		// separator row height
	int rssExpSegWidth;	// Number of segments for RSS Expdanded (Stacked)
	int ucc128linHeight;		// height of UCC/EAN-128 in X
	int dmRows;		// Data Matrix fixed number of rows
	int dmCols;		// Data Matrix fixed number of columns
	int qrVersion;		// QR Code fixed symbol version
	int qrEClevel;		// QR Code error correction level
	int format;		// BMP, TIF or RAW
	bool fileInputFlag;	// true is dataFile else dataStr
	char dataStr[MAX_DATA+1];
	char dataFile[MAX_FNAME+1];
	char outFile[MAX_FNAME+1];
	uint8_t *buffer;	// We may allocate an output buffer
	int bufferWidth;	// Width of a raw format buffer
	int bufferHeight;	// Height of a raw format buffer
	char **bufferStrings;	// We may allocate output as a set of strings
	char VERSION[16];

	// per-instance globals
	FILE *outfp;
	size_t bufferCap;
	size_t bufferSize;
	int errFlag;
	char errMsg[512];
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
	int rssexp_rowWidth;
	struct sPrints rsslim_prntSep;
	uint8_t rsslim_sepPattern[RSSLIM_SYM_W];
	struct sPrints rssutil_prntSep;
	uint8_t rssutil_sepPattern[MAX_SEP_ELMNTS];
	int rss_util_widths[MAX_K];
	uint8_t ucc128_patCCC[UCC128_MAX_PAT];

	// Ephemeral working space that can never clash
	union {
		struct patternLength qr_pats[MAX_QR_SIZE];
		struct patternLength dm_pats[MAX_DM_ROWS];
	};

};


#ifdef UNIT_TESTS

void test_api_getVersion(void);
void test_api_instanceSize(void);
void test_api_instanceSize(void);
void test_api_maxUcc128LinHeight(void);
void test_api_maxFilenameLength(void);
void test_api_maxInputBuffer(void);
void test_api_maxPixMult(void);
void test_api_defaults(void);
void test_api_sym(void);
void test_api_fileInputFlag(void);
void test_api_pixMult(void);
void test_api_XYundercut(void);
void test_api_sepHt(void);
void test_api_segWidth(void);
void test_api_linHeight(void);
void test_api_dmRowsColumns(void);
void test_api_qrVersion(void);
void test_api_qrEClevel(void);
void test_api_addCheckDigit(void);
void test_api_outFile(void);
void test_api_dataFile(void);
void test_api_dataStr(void);
void test_api_format(void);
void test_api_getBuffer(void);

#endif


#endif /* ENC_PRIVATE_H */
