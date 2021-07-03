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

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "enc-private.h"
#include "debug.h"
#include "dm.h"
#include "mtx.h"
#include "driver.h"


/*
 *  Definition of the symbol properties for each version
 *
 */

#define METRIC(r, c, rh, rv, cw, bl)		\
	{ .rows = r, .cols = c,			\
	  .regh = rh, .regv = rv,		\
	  .rscw = cw, .rsbl = bl,		\
	  .mrows = r-2*rh,			\
	  .mcols = c-2*rv,			\
	  .ncws = (r-2*rh)*(c-2*rv)/8 - cw,	\
	}


struct metric {
	uint8_t rows;		// Number of rows excluding QZ
	uint8_t cols;		// Number of columns excluding QZ
	uint8_t regh;		// Number of regions between horizontal timing patterns
	uint8_t regv;		// Number of regions between vertical timing patterns
	uint16_t rscw;		// Number of RSEC codewords
	uint8_t rsbl;		// Number of block for RSEC
	uint8_t mrows;		// Number of rows excluding timing patterns
	uint8_t mcols;		// Number of columns excluding timing patterns
	uint16_t ncws;		// Number of data codewords
};


static const struct metric metrics[30] = {
	//     rows  cols  regh  regv  rscw  rsbl
	METRIC(  10,   10,    1,    1,    5,    1),
	METRIC(  12,   12,    1,    1,    7,    1),
	METRIC(  14,   14,    1,    1,   10,    1),
	METRIC(  16,   16,    1,    1,   12,    1),
	METRIC(  18,   18,    1,    1,   14,    1),
	METRIC(  20,   20,    1,    1,   18,    1),
	METRIC(  22,   22,    1,    1,   20,    1),
	METRIC(  24,   24,    1,    1,   24,    1),
	METRIC(  26,   26,    1,    1,   28,    1),
	METRIC(  32,   32,    2,    2,   36,    1),
	METRIC(  36,   36,    2,    2,   42,    1),
	METRIC(  40,   40,    2,    2,   48,    1),
	METRIC(  44,   44,    2,    2,   56,    1),
	METRIC(  48,   48,    2,    2,   68,    1),
	METRIC(  52,   52,    2,    2,   84,    2),
	METRIC(  64,   64,    4,    4,  112,    2),
	METRIC(  72,   72,    4,    4,  144,    4),
	METRIC(  80,   80,    4,    4,  192,    4),
	METRIC(  88,   88,    4,    4,  224,    4),
	METRIC(  96,   96,    4,    4,  272,    4),
	METRIC( 104,  104,    4,    4,  336,    6),
	METRIC( 120,  120,    6,    6,  408,    6),
	METRIC( 132,  132,    6,    6,  496,    8),
	METRIC( 144,  144,    6,    6,  620,   10),
	METRIC(   8,   18,    1,    1,    7,    1),
	METRIC(   8,   32,    1,    2,   11,    1),
	METRIC(  12,   26,    1,    1,   14,    1),
	METRIC(  12,   36,    1,    2,   18,    1),
	METRIC(  16,   36,    1,    2,   24,    1),
	METRIC(  16,   48,    1,    2,   28,    1),
};


// Reed Solomon log table in GF(256)
static const uint8_t rslog[256] = {
	  0,    // undefined
	     255,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
	  4, 195,  39, 114, 227, 106, 134,  28, 243, 140,  44,  23,  55, 118, 211, 234,
	  5, 219, 196,  96,  40, 222, 115, 103, 228,  78, 107, 125, 135,   8,  29, 162,
	244, 186, 141, 180,  45,  99,  24,  49,  56,  13, 119, 153, 212, 199, 235,  91,
	  6,  76, 220, 217, 197,  11,  97, 184,  41,  36, 223, 253, 116, 138, 104, 193,
	229,  86,  79, 171, 108, 165, 126, 145, 136,  34,   9,  74,  30,  32, 163,  84,
	245, 173, 187, 204, 142,  81, 181, 190,  46,  88, 100, 159,  25, 231,  50, 207,
	 57, 147,  14,  67, 120, 128, 154, 248, 213, 167, 200,  63, 236, 110,  92, 176,
	  7, 161,  77, 124, 221, 102, 218,  95, 198,  90,  12, 152,  98,  48, 185, 179,
	 42, 209,  37, 132, 224,  52, 254, 239, 117, 233, 139,  22, 105,  27, 194, 113,
	230, 206,  87, 158,  80, 189, 172, 203, 109, 175, 166,  62, 127, 247, 146,  66,
	137, 192,  35, 252,  10, 183,  75, 216,  31,  83,  33,  73, 164, 144,  85, 170,
	246,  65, 174,  61, 188, 202, 205, 157, 143, 169,  82,  72, 182, 215, 191, 251,
	 47, 178,  89, 151, 101,  94, 160, 123,  26, 112, 232,  21,  51, 238, 208, 131,
	 58,  69, 148,  18,  15,  16,  68,  17, 121, 149, 129,  19, 155,  59, 249,  70,
	214, 250, 168,  71, 201, 156,  64,  60, 237, 130, 111,  20,  93, 122, 177, 150,
};


// Reed Solomon anti-log table in GF(256)
static const uint8_t rsalog[256] = {
	  1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
	229, 231, 227, 235, 251, 219, 155,  27,  54, 108, 216, 157,  23,  46,  92, 184,
	 93, 186,  89, 178,  73, 146,   9,  18,  36,  72, 144,  13,  26,  52, 104, 208,
	141,  55, 110, 220, 149,   7,  14,  28,  56, 112, 224, 237, 247, 195, 171, 123,
	246, 193, 175, 115, 230, 225, 239, 243, 203, 187,  91, 182,  65, 130,  41,  82,
	164, 101, 202, 185,  95, 190,  81, 162, 105, 210, 137,  63, 126, 252, 213, 135,
	 35,  70, 140,  53, 106, 212, 133,  39,  78, 156,  21,  42,  84, 168, 125, 250,
	217, 159,  19,  38,  76, 152,  29,  58, 116, 232, 253, 215, 131,  43,  86, 172,
	117, 234, 249, 223, 147,  11,  22,  44,  88, 176,  77, 154,  25,  50, 100, 200,
	189,  87, 174, 113, 226, 233, 255, 211, 139,  59, 118, 236, 245, 199, 163, 107,
	214, 129,  47,  94, 188,  85, 170, 121, 242, 201, 191,  83, 166,  97, 194, 169,
	127, 254, 209, 143,  51, 102, 204, 181,  71, 142,  49,  98, 196, 165, 103, 206,
	177,  79, 158,  17,  34,  68, 136,  61, 122, 244, 197, 167,  99, 198, 161, 111,
	222, 145,  15,  30,  60, 120, 240, 205, 183,  67, 134,  33,  66, 132,  37,  74,
	148,   5,  10,  20,  40,  80, 160, 109, 218, 153,  31,  62, 124, 248, 221, 151,
	  3,   6,  12,  24,  48,  96, 192, 173, 119, 238, 241, 207, 179,  75, 150,   1,
};


// Reed Solomon product in GF(256)
static inline uint8_t rsProd(const uint8_t a, const uint8_t b) {
	return a && b ? rsalog[ (rslog[a] + rslog[b]) % 255 ] : 0;
}


// Generate Reed Solomon coefficients using a generator 2
static void rsGenerateCoeffs(const int size, uint8_t *coeffs) {

	int i, j;

	assert(size >= 1);  // Satisfy static analysis

	coeffs[0] = 1;
	for (i = 1; i <= size; i++) {
		coeffs[i] = coeffs[i-1];
		for (j = i-1; j > 0; j--)
			coeffs[j] = coeffs[j-1] ^ rsProd(coeffs[j], rsalog[i]);
		coeffs[0] = rsProd(coeffs[0], rsalog[i]);
	}

}


// Perform Reed Solomon ECC codeword calculation
static void rsEncode(const uint8_t* datcws, const int datlen, uint8_t* ecccws, const int ecclen, const uint8_t* coeffs) {

	int i, j;
	uint8_t tmp[MAX_DM_DAT_CWS_PER_BLK + MAX_DM_ECC_CWS_PER_BLK] = { 0 };

	assert(datlen <= MAX_DM_DAT_CWS_PER_BLK);
	assert(ecclen <= MAX_DM_ECC_CWS_PER_BLK);

	memcpy(tmp, datcws, (size_t)datlen);

	for (i = 0; i < datlen; i++)
		for (j = 0; j < ecclen; j++)
			tmp[i+j+1] = rsProd(coeffs[ecclen-j-1], tmp[i]) ^ tmp[i+j+1];

	memcpy(ecccws, tmp + datlen, (size_t)ecclen);

}


// Generate the codeword sequence that represents the data message
static void createCodewords(gs1_encoder *ctx, const uint8_t *string, uint8_t cws[MAX_DM_CWS], uint16_t* cwslen) {

	uint8_t *p;
	const uint8_t *q;
	bool gs1Mode = false;

	(void)ctx;

	if (*string == '#') {		// "#..." => GS1 mode
		gs1Mode = true;
	} else {
		// Unescape leading sequence "\\...#" -> "\...#"
		q = string;
		while (*q == '\\')
			q++;
		if (*q == '#')
			string++;
	}

	// Encode the message in ASCII mode
	p = cws;
	while (*string && p-cws < MAX_DM_DAT_CWS) {
		if (*string == '#' && gs1Mode) {
			*p++ = 232;
			string++;
		} else if (*string >= '0' && *string <= '9') {
			if (*(string+1) && *(string+1) >= '0' && *(string+1) <= '9') {
				*p++ = (uint8_t)(((*string)-'0')*10 + *(string+1)-'0' + 130);
				string += 2;
			} else {  // Single digit
				*p++ = (uint8_t)((*string++) + 1);
			}
		} else if (*string <= 127) {
			*p++ = (uint8_t)((*string++) + 1);
		} else {  // Extended ASCII
			*p++ = 235;
			*p++ = (uint8_t)((*string++) - 127);
		}
	}

	*cwslen = (!*string && p-cws <= MAX_DM_DAT_CWS) ? (uint16_t)(p-cws) : UINT16_MAX;

}


// Select a symbol version that is sufficent to hold the encoded bitstream
static const struct metric* selectVersion(gs1_encoder *ctx, const uint16_t cwslen) {

	const struct metric *m = NULL;
	bool okay;
	int vers;

	// Select a suitable symbol
	for (vers = 1; vers < (int)(SIZEOF_ARRAY(metrics)); vers++) {
		m = &metrics[vers];
		okay = true;
		if (ctx->dmRows != 0 &&
		    ctx->dmRows != m->rows) okay = false;     // User specified rows
		if (ctx->dmCols != 0 &&
		    ctx->dmCols != m->cols) okay = false;     // User specified columns
		if (cwslen > m->ncws) okay = false;              // Bitstream must fit capacity of symbol
		if (okay) break;
	}

	return okay ? m : NULL;

}


// Add pseudo-random padding codewords to the bitstream then perform Reed
// Solomon Error Correction
static void finaliseCodewords(gs1_encoder *ctx, uint8_t *cws, uint16_t *cwslen, const struct metric *m) {

	uint8_t tmpcws[MAX_DM_DAT_CWS_PER_BLK+MAX_DM_ECC_CWS_PER_BLK] = { 0 };
	uint8_t coeffs[MAX_DM_ECC_CWS_PER_BLK+1];
	int i, j, pad, offset;
	uint8_t *p;

	(void)ctx;

	assert(*cwslen <= m->ncws);

	// Complete the message by adding pseudo-random padding codewords
	p = cws + *cwslen;
	if (p-cws < m->ncws)
		*p++ = 129;
	while (p-cws < m->ncws) {
		pad = (uint8_t)((p-cws+1)*149 % 253 + 130);
		if (pad > 254) pad -= 254;
		*p++ = (uint8_t)pad;
	}

	DEBUG_PRINT_CWS("Padded", cws, (uint16_t)(p-cws));

	// Generate coefficients
	rsGenerateCoeffs(m->rscw / m->rsbl, coeffs);

	// Error correction for interleaved blocks of codewords
	for (i = 0; i < m->rsbl; i++) {

		p = tmpcws;
		for (j = i; j < m->ncws; j += m->rsbl)
			*p++ = cws[j];

		rsEncode(tmpcws, (int)(p-tmpcws), p, m->rscw/m->rsbl, coeffs);

		offset = m->rscw == 620 ? (i<8 ? 2:-8) : 0;
		for (j = i; j < m->rscw; j += m->rsbl)
			cws[m->ncws + j + offset] = *p++;

	}

}


#define putTimingModule(c,r,b) do {							\
	assert(c >= 0 && c < m->cols);							\
	assert(r >= 0 && r < m->rows);							\
	gs1_mtxPutModule(mtx, m->cols + 2*DM_QZ,					\
			 DM_QZ + c, DM_QZ + r,						\
			 b);								\
} while(0)


// Place a data module, handling wrapping and QZ, jumping fixtures and mark it
// as reserved
#define putModule(cx,rx,b) do {								\
	int cc = cx; int rr = rx;							\
	assert(m->mcols >= 8);								\
	assert(m->mrows >= 6);								\
	if (rr < 0)         { rr += m->mrows; cc += 4-(m->mrows+4)%8; }			\
	if (cc < 0)         { cc += m->mcols; rr += 4-(m->mcols+4)%8; }			\
	if (rr >= m->mrows) { rr = rr % m->mrows;                     }			\
	assert(cc >= 0 && cc < m->mcols);						\
	assert(rr >= 0 && rr < m->mrows);						\
	gs1_mtxPutModule(occ, m->mcols, cc, rr, 1);					\
	gs1_mtxPutModule(mtx, m->cols + 2*DM_QZ,					\
			 DM_QZ + cc + 2*(cc/(m->mcols/m->regv)) + 1,			\
			 DM_QZ + rr + 2*(rr/(m->mrows/m->regh)) + 1,			\
			 b);								\
} while(0)


// Place a codeword in the matrix in the typical "b" pattern, possibly wrapped
#define plotCodeword(c1,r1,c2,r2,c3,r3,c4,r4,c5,r5,c6,r6,c7,r7,c8,r8) do {		\
	putModule(c1, r1, *cws >>7 & 1);						\
	putModule(c2, r2, *cws >>6 & 1);						\
	putModule(c3, r3, *cws >>5 & 1);						\
	putModule(c4, r4, *cws >>4 & 1);						\
	putModule(c5, r5, *cws >>3 & 1);						\
	putModule(c6, r6, *cws >>2 & 1);						\
	putModule(c7, r7, *cws >>1 & 1);						\
	putModule(c8, r8, *cws >>0 & 1);						\
	cws++;										\
} while(0)


// Place a codeword in the matrix at a corner where the wrapping is atypical
#define plotCodewordCorner(c1,r1,c2,r2,c3,r3,c4,r4,c5,r5,c6,r6,c7,r7,c8,r8) do {	\
	plotCodeword(c1 >= 0 ? c1 : c1 + m->mcols, r1 >= 0 ? r1 : r1 + m->mrows,	\
		     c2 >= 0 ? c2 : c2 + m->mcols, r2 >= 0 ? r2 : r2 + m->mrows,	\
		     c3 >= 0 ? c3 : c3 + m->mcols, r3 >= 0 ? r3 : r3 + m->mrows,	\
		     c4 >= 0 ? c4 : c4 + m->mcols, r4 >= 0 ? r4 : r4 + m->mrows,	\
		     c5 >= 0 ? c5 : c5 + m->mcols, r5 >= 0 ? r5 : r5 + m->mrows,	\
		     c6 >= 0 ? c6 : c6 + m->mcols, r6 >= 0 ? r6 : r6 + m->mrows,	\
		     c7 >= 0 ? c7 : c7 + m->mcols, r7 >= 0 ? r7 : r7 + m->mrows,	\
		     c8 >= 0 ? c8 : c8 + m->mcols, r8 >= 0 ? r8 : r8 + m->mrows);	\
} while(0)


// Create a symbol that holds the given bitstream
static void createMatrix(gs1_encoder *ctx, uint8_t *mtx, const uint8_t *cws, const struct metric *m) {

	uint8_t occ[MAX_DM_BYTES] = { 0 };  // Matrix to indicate occupied positions
	int i, j;

	(void)ctx;

	// Plot timing patterns
	for (i = 0; i < m->cols + 1; i += m->mcols / m->regv + 2) {
		for (j = 0; j < m->rows; j++) {
			if (i > 0)
				putTimingModule(i-1, j, (uint8_t)(j%2));
			if (i < m->cols)
				putTimingModule(i, j, 1);
		}
	}
	for (j = 0; j < m->rows + 1; j += m->mrows / m->regh + 2) {
		for (i = 0; i < m->cols; i++) {
			if (j > 0)
				putTimingModule(i, j-1, 1);
			if (j < m->rows)
				putTimingModule(i, j, (uint8_t)(1-i%2));
		}
	}


	// Place the modules between the timing patterns
	i = 0; j = 4;

	do {

		if (i == 0 && j == m->mrows)
			plotCodewordCorner(
				0,-1,	1,-1,	2,-1,	/**/
				/***********************/  /****************/
				/***********************/  /****************/
							/**/	-2,0,	-1,0,
							/**/	-1,1,
							/**/	-1,2,
							/**/	-1,3
			);
		if (i == 0 && j == m->mrows-2 && m->mcols % 4 != 0)
			plotCodewordCorner(
				0,-3,	/**/
				0,-2,	/**/
				0,-1,	/**/
				/*******/  /********************************/
				/*******/  /********************************/
					/**/	-4,0,	-3,0,	-2,0,	-1,0,
					/**/				-1,1
			);
		if (i == 0 && j == m->mrows - 2 && m->mcols % 8 == 4)
			plotCodewordCorner(
				0,-3,	/**/
				0,-2,	/**/
				0,-1,	/**/
				/*******/  /****************/
				/*******/  /****************/
					/**/	-2,0,	-1,0,
					/**/		-1,1,
					/**/		-1,2,
					/**/		-1,3
			);
		if (i == 2 && j == m->mrows + 4 && m->mcols % 8 == 0)
			plotCodewordCorner(
				0,-1,	/**/			-1,-1,
				/*******/  /*************************/
				/*******/  /*************************/
					/**/	-3,0,	-2,0,	-1,0,
					/**/	-3,1,	-2,1,	-1,1
			);

		// Sweep upwards
		do {
			if (i >= 0 && j < m->mrows && !gs1_mtxGetModule(occ, m->mcols, i, j)) {
				plotCodeword(
					i-2,j-2,  i-1,j-2,
					i-2,j-1,  i-1,j-1,  i-0,j-1,
					i-2,j-0,  i-1,j-0,  i-0,j-0
				);
			}
			i+=2; j-=2;
		} while (i < m->mcols && j >= 0);
		i+=3; j++;

		// Sweep downwards
		do {
			if (i < m->mcols && j >= 0 &&
			    !gs1_mtxGetModule(occ, m->mcols, i, j))
				plotCodeword(
					i-2,j-2,  i-1,j-2,
					i-2,j-1,  i-1,j-1,  i-0,j-1,
					i-2,j-0,  i-1,j-0,  i-0,j-0
				);
			i-=2; j+=2;
		} while (i >= 0 && j < m->mrows);
		i++; j+=3;

	} while (i < m->mcols || j < m->mrows);


	// Set checker pattern if required
	if (gs1_mtxGetModule(occ, m->mcols, m->mrows-1, m->mcols-1) == 0) {
		putModule(m->mrows - 2, m->mcols - 2, 1);
		putModule(m->mrows - 1, m->mcols - 2, 0);
		putModule(m->mrows - 2, m->mcols - 1, 0);
		putModule(m->mrows - 1, m->mcols - 1, 1);
	}

}


static int DMenc(gs1_encoder *ctx, const uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_DM_BYTES] = { 0 };
	uint8_t cws[MAX_DM_CWS] = { 0 };
	uint16_t cwslen = 0;
	const struct metric *m;

	(void)ctx;
	(void)string;

	DEBUG_PRINT("\nData: %s\n", string);

	if (*string == '#' && strchr((char*)string, '|') != NULL) {
		strcpy(ctx->errMsg, "Composite component is not supported for Data Matrix");
		ctx->errFlag = true;
		return 0;
	}

	// For GS1 purposes we restrict to AI or DL only
	if (!(*string == '#' ||
	     (strlen((char*)string) >= 8 && strncmp((char*)string, "https://", 8) == 0) ||
	     (strlen((char*)string) >= 7 && strncmp((char*)string, "http://",  7) == 0)) ) {
		strcpy(ctx->errMsg, "Data Matrix input must be either an AI element string or a Digital Link URI");
		ctx->errFlag = true;
		return 0;
	}

	createCodewords(ctx, string, cws, &cwslen);
	if (cwslen == UINT16_MAX) {
		strcpy(ctx->errMsg, "Data exceeds the capacity of any Data Matrix symbol");
		ctx->errFlag = true;
		return 0;
	}

	assert(cwslen <= MAX_DM_DAT_CWS);

	DEBUG_PRINT_CWS("Codewords", cws, cwslen);

	m = selectVersion(ctx, cwslen);
	if (!m) {
		strcpy(ctx->errMsg, "Data exceeds the capacity of the specified symbol");
		ctx->errFlag = true;
		return 0;
	}

	DEBUG_PRINT("Symbol: %dx%d (cws: %d; ecc: %d; blocks: %d; regv: %d; regh: %d)\n",
		m->rows, m->cols, m->ncws, m->rscw, m->rsbl, m->regh, m->regv);

	finaliseCodewords(ctx, cws, &cwslen, m);

	assert(cwslen <= MAX_DM_CWS);

	DEBUG_PRINT_CWS("ECC codewords", cws + m->ncws, m->rscw);

	createMatrix(ctx, mtx, cws, m);

	DEBUG_PRINT_MATRIX("Matrix", mtx, m->cols + 2*DM_QZ, m->rows + 2*DM_QZ);

	gs1_mtxToPatterns(mtx, m->cols + 2*DM_QZ, m->rows + 2*DM_QZ, pats);

	DEBUG_PRINT_PATTERN_LENGTHS("Patterns", pats, m->rows);

	return m->rows + 2*DM_QZ;

}


void gs1_DM(gs1_encoder *ctx) {

	struct sPrints prints;
	struct patternLength *pats;
	char* dataStr = ctx->dataStr;
	int rows, cols, i;

	pats = ctx->dm_pats;

	if (!(rows = DMenc(ctx, (uint8_t*)dataStr, pats)) || ctx->errFlag)
		goto out;

	cols = 0;
	for (i = 0; i < pats[0].length; i++)
		cols += pats[0].pattern[i];

	gs1_driverInit(ctx, ctx->pixMult*cols, ctx->pixMult*rows);

	ctx->line1 = true; // so first line is not Y undercut
	prints.height = ctx->pixMult;
	prints.guards = false;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.reverse = false;

	for (i = 0; i < rows; i++) {
		prints.elmCnt = pats[i].length;
		prints.pattern = pats[i].pattern;
		prints.whtFirst = pats[i].whtFirst;
		gs1_driverAddRow(ctx, &prints);
	}

	gs1_driverFinalise(ctx);

out:

	return;

}


#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_dm_DM_encode(void) {

	const char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (const char*[]){
"                        ",
" X X X X X X X X X X X  ",
" X XX XXX  X X X X X  X ",
" XXX  X  XXX  XXXX      ",
" XX XXX  XX  XX X XXX X ",
" XX  X  X X X   X X XX  ",
" XXXXX     XX   X   XXX ",
" X  XXX X X XXX X XX    ",
" XX  XXXXX XX  XXXXX XX ",
" X  X              XX   ",
" X XX XXX X   X X X   X ",
" XX XX    X   XXX XXX   ",
" X X    X  XX  X XXX XX ",
" X    XX XXX XX  X X    ",
" X XX X   XX   XX     X ",
" X    X    X X  X X     ",
" X XX   X XXXX   XXX  X ",
" XX   XXXXX X XX XX XX  ",
" XX XX X XX XXX       X ",
" X XXXX    X X  X X  X  ",
" X  X  X X XXXXX  X X X ",
" XX X   XX  XXXXXX   X  ",
" XXXXXXXXXXXXXXXXXXXXXX ",
"                        ",
NULL
	};
	TEST_CHECK(test_encode(ctx, true, gs1_encoder_sDM, "https://id.gs1.org/01/12312312312333", expect));

	expect = (const char*[]){
"                      ",
" X X X X X X X X X X  ",
" XX XX X   XXX XX XXX ",
" X   X X     XXX  X   ",
" X XXXXX XX  X X    X ",
" XXXX X    X       X  ",
" X  X  X XX  XXXXX  X ",
" XX      X X X   XXX  ",
" XXXX  X  X     XXX X ",
" X    XX XXXX X   XX  ",
" XX X    X XXXXX  X X ",
" XX  X    XX X XXX    ",
" X  XX     X XXXXXXXX ",
" XXX      XX  X X     ",
" X    XXX   XXXX X XX ",
" XX  XX   XXXX XX XX  ",
" XX   XXX XXX  X   XX ",
" X  XXXX     XX X     ",
" XX    XXXX XX  XXX X ",
" X  XX X    X  X X X  ",
" XXXXXXXXXXXXXXXXXXXX ",
"                      ",
NULL
	};
	TEST_CHECK(test_encode(ctx, true, gs1_encoder_sDM, "#011234567890123110ABC123#11210630", expect));  // GS1 mode


	TEST_CHECK(test_encode(ctx, false, gs1_encoder_sDM, "#0112345678901231|#99ABC", NULL));  // CC is invalid


	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
