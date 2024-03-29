/**
 * GS1 Barcode Engine
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "enc-private.h"
#include "debug.h"
#include "qr.h"
#include "mtx.h"
#include "driver.h"


/*
 *  Definition of the symbol properties for each version
 *
 */

#define METRIC(v, vg, s, a2, a3, m, el, em, eq, eh, l1, l2, m1, m2, q1, q2, h1, h2) {	\
	   .version = v,								\
	   .vergrp = vg,								\
	   .size = s,									\
	   .align = { a2, a3 },								\
	   .modules = m,								\
	   .ecc_cws = { el, em, eq, eh }, 						\
	   .ecc_blks = { {l1,l2}, {m1,m2}, {q1,q2}, {h1,h2} },				\
	}


struct metric {
	uint8_t version;		// Symbol version
	uint8_t vergrp;			// Version group: Data encoding differs between sizes
	uint8_t size;			// Length of a side
	uint8_t align[2];		// Position of the second and third alignment patterns
	uint16_t modules;		// Total number of modules for data and ECC
	uint16_t ecc_cws[4];		// Number of ECC codewords for each ECC level
	uint8_t ecc_blks[4][2];		// Number of ECC blocks in two groups for each ECC level
};


static const struct metric metrics[41] = {
	{0},  // spacer so that metrics are indexed at version number

	// version  vergrp  size    align  modules           ecc_cws                    ecc_blks
	//                                             L     M     Q     H   L1  L2  M1  M2  Q1  Q2  H1  H2
	METRIC(  1,      0,   21,  98, 99,     208,    7,   10,   13,   17,   1,  0,  1,  0,  1,  0,  1,  0),
	METRIC(  2,      0,   25,  18, 99,     359,   10,   16,   22,   28,   1,  0,  1,  0,  1,  0,  1,  0),
	METRIC(  3,      0,   29,  22, 99,     567,   15,   26,   36,   44,   1,  0,  1,  0,  2,  0,  2,  0),
	METRIC(  4,      0,   33,  26, 99,     807,   20,   36,   52,   64,   1,  0,  2,  0,  2,  0,  4,  0),
	METRIC(  5,      0,   37,  30, 99,    1079,   26,   48,   72,   88,   1,  0,  2,  0,  2,  2,  2,  2),
	METRIC(  6,      0,   41,  34, 99,    1383,   36,   64,   96,  112,   2,  0,  4,  0,  4,  0,  4,  0),
	METRIC(  7,      0,   45,  22, 38,    1568,   40,   72,  108,  130,   2,  0,  4,  0,  2,  4,  4,  1),
	METRIC(  8,      0,   49,  24, 42,    1936,   48,   88,  132,  156,   2,  0,  2,  2,  4,  2,  4,  2),
	METRIC(  9,      0,   53,  26, 46,    2336,   60,  110,  160,  192,   2,  0,  3,  2,  4,  4,  4,  4),
	METRIC( 10,      1,   57,  28, 50,    2768,   72,  130,  192,  224,   2,  2,  4,  1,  6,  2,  6,  2),
	METRIC( 11,      1,   61,  30, 54,    3232,   80,  150,  224,  264,   4,  0,  1,  4,  4,  4,  3,  8),
	METRIC( 12,      1,   65,  32, 58,    3728,   96,  176,  260,  308,   2,  2,  6,  2,  4,  6,  7,  4),
	METRIC( 13,      1,   69,  34, 62,    4256,  104,  198,  288,  352,   4,  0,  8,  1,  8,  4, 12,  4),
	METRIC( 14,      1,   73,  26, 46,    4651,  120,  216,  320,  384,   3,  1,  4,  5, 11,  5, 11,  5),
	METRIC( 15,      1,   77,  26, 48,    5243,  132,  240,  360,  432,   5,  1,  5,  5,  5,  7, 11,  7),
	METRIC( 16,      1,   81,  26, 50,    5867,  144,  280,  408,  480,   5,  1,  7,  3, 15,  2,  3, 13),
	METRIC( 17,      1,   85,  30, 54,    6523,  168,  308,  448,  532,   1,  5, 10,  1,  1, 15,  2, 17),
	METRIC( 18,      1,   89,  30, 56,    7211,  180,  338,  504,  588,   5,  1,  9,  4, 17,  1,  2, 19),
	METRIC( 19,      1,   93,  30, 58,    7931,  196,  364,  546,  650,   3,  4,  3, 11, 17,  4,  9, 16),
	METRIC( 20,      1,   97,  34, 62,    8683,  224,  416,  600,  700,   3,  5,  3, 13, 15,  5, 15, 10),
	METRIC( 21,      1,  101,  28, 50,    9252,  224,  442,  644,  750,   4,  4, 17,  0, 17,  6, 19,  6),
	METRIC( 22,      1,  105,  26, 50,   10068,  252,  476,  690,  816,   2,  7, 17,  0,  7, 16, 34,  0),
	METRIC( 23,      1,  109,  30, 54,   10916,  270,  504,  750,  900,   4,  5,  4, 14, 11, 14, 16, 14),
	METRIC( 24,      1,  113,  28, 54,   11796,  300,  560,  810,  960,   6,  4,  6, 14, 11, 16, 30,  2),
	METRIC( 25,      1,  117,  32, 58,   12708,  312,  588,  870, 1050,   8,  4,  8, 13,  7, 22, 22, 13),
	METRIC( 26,      1,  121,  30, 58,   13652,  336,  644,  952, 1110,  10,  2, 19,  4, 28,  6, 33,  4),
	METRIC( 27,      2,  125,  34, 62,   14628,  360,  700, 1020, 1200,   8,  4, 22,  3,  8, 26, 12, 28),
	METRIC( 28,      2,  129,  26, 50,   15371,  390,  728, 1050, 1260,   3, 10,  3, 23,  4, 31, 11, 31),
	METRIC( 29,      2,  133,  30, 54,   16411,  420,  784, 1140, 1350,   7,  7, 21,  7,  1, 37, 19, 26),
	METRIC( 30,      2,  137,  26, 52,   17483,  450,  812, 1200, 1440,   5, 10, 19, 10, 15, 25, 23, 25),
	METRIC( 31,      2,  141,  30, 56,   18587,  480,  868, 1290, 1530,  13,  3,  2, 29, 42,  1, 23, 28),
	METRIC( 32,      2,  145,  34, 60,   19723,  510,  924, 1350, 1620,  17,  0, 10, 23, 10, 35, 19, 35),
	METRIC( 33,      2,  149,  30, 58,   20891,  540,  980, 1440, 1710,  17,  1, 14, 21, 29, 19, 11, 46),
	METRIC( 34,      2,  153,  34, 62,   22091,  570, 1036, 1530, 1800,  13,  6, 14, 23, 44,  7, 59,  1),
	METRIC( 35,      2,  157,  30, 54,   23008,  570, 1064, 1590, 1890,  12,  7, 12, 26, 39, 14, 22, 41),
	METRIC( 36,      2,  161,  24, 50,   24272,  600, 1120, 1680, 1980,   6, 14,  6, 34, 46, 10,  2, 64),
	METRIC( 37,      2,  165,  28, 54,   25568,  630, 1204, 1770, 2100,  17,  4, 29, 14, 49, 10, 24, 46),
	METRIC( 38,      2,  169,  32, 58,   26896,  660, 1260, 1860, 2220,   4, 18, 13, 32, 48, 14, 42, 32),
	METRIC( 39,      2,  173,  26, 54,   28256,  720, 1316, 1950, 2310,  20,  4, 40,  7, 43, 22, 10, 67),
	METRIC( 40,      2,  177,  30, 58,   29648,  750, 1372, 2040, 2430,  19,  6, 18, 31, 34, 34, 20, 61),
};


// Character count length for each mode within each group
static const uint8_t cclens[3][4] = {
//         N   A   B   K         vergrp
	{ 10,  9,  8,  8 },    //  1-9
	{ 12, 11, 16, 10 },    // 10-26
	{ 14, 13, 16, 12 },    // 27-40
};


// Finder pattern bitmap
static const uint8_t finder[8][8] = {
	{ 1,1,1,1,1,1,1,0 },
	{ 1,0,0,0,0,0,1,0 },
	{ 1,0,1,1,1,0,1,0 },
	{ 1,0,1,1,1,0,1,0 },
	{ 1,0,1,1,1,0,1,0 },
	{ 1,0,0,0,0,0,1,0 },
	{ 1,1,1,1,1,1,1,0 },
	{ 0,0,0,0,0,0,0,0 },
};


// Alignment pattern bitmap
static const uint8_t algnpat[5][5] = {
	{ 1,1,1,1,1 },
	{ 1,0,0,0,1 },
	{ 1,0,1,0,1 },
	{ 1,0,0,0,1 },
	{ 1,1,1,1,1 },
};


/*
 * Coordinate system (i,j) is indexed with (1,1) at top-left.  Negative
 * cooridinates wrap around to the other extent of the same row/column, such
 * that (-1,-1) is at bottom-right. The coordinate system is oblivious to the
 * quiet zone.
 *
 */


// Position of the format information modules
static const int8_t formatpos[15][2][2] = {
	{ {  1, 9 }, {  9,-1 } },  { {  2, 9 }, {  9,-2 } },  { {  3, 9 }, {  9,-3 } },
	{ {  4, 9 }, {  9,-4 } },  { {  5, 9 }, {  9,-5 } },  { {  6, 9 }, {  9,-6 } },
	{ {  8, 9 }, {  9,-7 } },  { {  9, 9 }, { -8, 9 } },  { {  9, 8 }, { -7, 9 } },
	{ {  9, 6 }, { -6, 9 } },  { {  9, 5 }, { -5, 9 } },  { {  9, 4 }, { -4, 9 } },
	{ {  9, 3 }, { -3, 9 } },  { {  9, 2 }, { -2, 9 } },  { {  9, 1 }, { -1, 9 } },
};


// Position of the version information modules
static const int8_t versionpos[18][2][2] = {
	{ {  -9,  6 }, {  6, -9 } },  { { -10,  6 }, {  6,-10 } },
	{ { -11,  6 }, {  6,-11 } },  { {  -9,  5 }, {  5, -9 } },
	{ { -10,  5 }, {  5,-10 } },  { { -11,  5 }, {  5,-11 } },
	{ {  -9,  4 }, {  4, -9 } },  { { -10,  4 }, {  4,-10 } },
	{ { -11,  4 }, {  4,-11 } },  { {  -9,  3 }, {  3, -9 } },
	{ { -10,  3 }, {  3,-10 } },  { { -11,  3 }, {  3,-11 } },
	{ {  -9,  2 }, {  2, -9 } },  { { -10,  2 }, {  2,-10 } },
	{ { -11,  2 }, {  2,-11 } },  { {  -9,  1 }, {  1, -9 } },
	{ { -10,  1 }, {  1,-10 } },  { { -11,  1 }, {  1,-11 } },
};


// Map of the format information value to (15,5) BCH code result.
static const uint16_t formatmap[32] = {
	0x5412, 0x5125, 0x5e7c, 0x5b4b, 0x45f9, 0x40ce, 0x4f97, 0x4aa0,
	0x77c4, 0x72f3, 0x7daa, 0x789d, 0x662f, 0x6318, 0x6c41, 0x6976,
	0x1689, 0x13be, 0x1ce7, 0x19d0, 0x0762, 0x0255, 0x0d0c, 0x083b,
	0x355f, 0x3068, 0x3f31, 0x3a06, 0x24b4, 0x2183, 0x2eda, 0x2bed,
};


// Map of version information value to (18,6) Golay code result.
static const uint32_t versionmap[34] = {
	0x07c94, 0x085bc, 0x09a99, 0x0a4d3, 0x0bbf6, 0x0c762, 0x0d847,   //  v7-13
	0x0e60d, 0x0f928, 0x10b78, 0x1145d, 0x12a17, 0x13532, 0x149a6,   // v14-20
	0x15683, 0x168c9, 0x177ec, 0x18ec4, 0x191e1, 0x1afab, 0x1b08e,   // v21-27
	0x1cc1a, 0x1d33f, 0x1ed75, 0x1f250, 0x209d5, 0x216fd, 0x228ba,   // v28-34
	0x2379f, 0x24b0b, 0x2542e, 0x26a64, 0x27541, 0x28c69,            // v35-40
};


// Reed Solomon log table in GF(256)
static const uint8_t rslog[256] = {
	  0,    // undefined
	     255,   1,  25,   2,  50,  26, 198,   3, 223,  51, 238,  27, 104, 199,  75,
	  4, 100, 224,  14,  52, 141, 239, 129,  28, 193, 105, 248, 200,   8,  76, 113,
	  5, 138, 101,  47, 225,  36,  15,  33,  53, 147, 142, 218, 240,  18, 130,  69,
	 29, 181, 194, 125, 106,  39, 249, 185, 201, 154,   9, 120,  77, 228, 114, 166,
	  6, 191, 139,  98, 102, 221,  48, 253, 226, 152,  37, 179,  16, 145,  34, 136,
	 54, 208, 148, 206, 143, 150, 219, 189, 241, 210,  19,  92, 131,  56,  70,  64,
	 30,  66, 182, 163, 195,  72, 126, 110, 107,  58,  40,  84, 250, 133, 186,  61,
	202,  94, 155, 159,  10,  21, 121,  43,  78, 212, 229, 172, 115, 243, 167,  87,
	  7, 112, 192, 247, 140, 128,  99,  13, 103,  74, 222, 237,  49, 197, 254,  24,
	227, 165, 153, 119,  38, 184, 180, 124,  17,  68, 146, 217,  35,  32, 137,  46,
	 55,  63, 209,  91, 149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190,  97,
	242,  86, 211, 171,  20,  42,  93, 158, 132,  60,  57,  83,  71, 109,  65, 162,
	 31,  45,  67, 216, 183, 123, 164, 118, 196,  23,  73, 236, 127,  12, 111, 246,
	108, 161,  59,  82,  41, 157,  85, 170, 251,  96, 134, 177, 187, 204,  62,  90,
	203,  89,  95, 176, 156, 169, 160,  81,  11, 245,  22, 235, 122, 117,  44, 215,
	 79, 174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168,  80,  88, 175,
};


// Reed Solomon anti-log table in GF(256)
static const uint8_t rsalog[256] = {
	  1,   2,   4,   8,  16,  32,  64, 128,  29,  58, 116, 232, 205, 135,  19,  38,
	 76, 152,  45,  90, 180, 117, 234, 201, 143,   3,   6,  12,  24,  48,  96, 192,
	157,  39,  78, 156,  37,  74, 148,  53, 106, 212, 181, 119, 238, 193, 159,  35,
	 70, 140,   5,  10,  20,  40,  80, 160,  93, 186, 105, 210, 185, 111, 222, 161,
	 95, 190,  97, 194, 153,  47,  94, 188, 101, 202, 137,  15,  30,  60, 120, 240,
	253, 231, 211, 187, 107, 214, 177, 127, 254, 225, 223, 163,  91, 182, 113, 226,
	217, 175,  67, 134,  17,  34,  68, 136,  13,  26,  52, 104, 208, 189, 103, 206,
	129,  31,  62, 124, 248, 237, 199, 147,  59, 118, 236, 197, 151,  51, 102, 204,
	133,  23,  46,  92, 184, 109, 218, 169,  79, 158,  33,  66, 132,  21,  42,  84,
	168,  77, 154,  41,  82, 164,  85, 170,  73, 146,  57, 114, 228, 213, 183, 115,
	230, 209, 191,  99, 198, 145,  63, 126, 252, 229, 215, 179, 123, 246, 241, 255,
	227, 219, 171,  75, 150,  49,  98, 196, 149,  55, 110, 220, 165,  87, 174,  65,
	130,  25,  50, 100, 200, 141,   7,  14,  28,  56, 112, 224, 221, 167,  83, 166,
	 81, 162,  89, 178, 121, 242, 249, 239, 195, 155,  43,  86, 172,  69, 138,   9,
	 18,  36,  72, 144,  61, 122, 244, 245, 247, 243, 251, 235, 203, 139,  11,  22,
	 44,  88, 176, 125, 250, 233, 207, 131,  27,  54, 108, 216, 173,  71, 142,   1,
};


// Mask functions
static inline uint8_t mask1(const uint8_t i, const uint8_t j) {
	return (uint8_t)((i+j)%2 == 0);
}

static inline uint8_t mask2(const uint8_t i, const uint8_t j) {
	(void)i;
	return (uint8_t)(j%2 == 0);
}

static inline uint8_t mask3(const uint8_t i, const uint8_t j) {
	(void)j;
	return (uint8_t)(i%3 == 0);
}

static inline uint8_t mask4(const uint8_t i, const uint8_t j) {
	return (uint8_t)((i+j)%3 == 0);
}

static inline uint8_t mask5(const uint8_t i, const uint8_t j) {
	return (uint8_t)((j/2+i/3)%2 == 0);
}

static inline uint8_t mask6(const uint8_t i, const uint8_t j) {
	return (uint8_t)((i*j)%2+(i*j)%3 == 0);
}

static inline uint8_t mask7(const uint8_t i, const uint8_t j) {
	return (uint8_t)(((i*j)%2+(i*j)%3)%2 == 0);
}

static inline uint8_t mask8(const uint8_t i, const uint8_t j) {
	return (uint8_t)(((i*j)%3+(i+j)%2)%2 == 0);
}

static uint8_t (*const maskfun[8]) (const uint8_t x, const uint8_t y) = {
	mask1, mask2, mask3, mask4, mask5, mask6, mask7, mask8
};


// Syntactic sugar to include qz offset and wrap around processing
#define putModule(d, x, y, b) do {						\
	assert( x != 0 && y != 0 && abs(x) <= m->size && abs(y) <= m->size );	\
	gs1_mtxPutModule(d, m->size + 2*QR_QZ,					\
		x > 0 ? x + QR_QZ - 1 : m->size + QR_QZ + x,			\
		y > 0 ? y + QR_QZ - 1 : m->size + QR_QZ + y,			\
		b);								\
} while(0)

#define getModule(s, x, y)							\
	gs1_mtxGetModule(s, m->size + 2*QR_QZ,					\
		x > 0 ? x + QR_QZ - 1 : m->size + QR_QZ + x,			\
		y > 0 ? y + QR_QZ - 1 : m->size + QR_QZ + y)

#define putFixtureModule(x, y, b) do {						\
	putModule(mtx, x, y, b);						\
	putModule(fix, x, y, 1);						\
} while(0)

#define putAlign(x, y) do {							\
	doPutAlign(mtx, fix, m, x, y);						\
} while(0)


// Place alignment pattern at a coordinate and mark as a fixture module
static void doPutAlign(uint8_t *mtx, uint8_t *fix, const struct metric *m, const int x, const int y) {

	int i, j;

	for (i = 0; i < (int)sizeof(algnpat[0]); i++)
		for (j = 0; j < (int)sizeof(algnpat[0]); j++)
			putFixtureModule(x+i, y+j, algnpat[i][j]);

}


// Reed Solomon product in GF(256)
static inline uint8_t rsProd(const uint8_t a, const uint8_t b) {
	return a && b ? rsalog[ (rslog[a] + rslog[b]) % 255 ] : 0;
}


// Generate Reed Solomon coefficients using a generator 2
static void rsGenerateCoeffs(const int size, uint8_t *coeffs) {

	int i, j;

	coeffs[0] = 1;
	for (i = 0; i < size; i++) {
		coeffs[i+1] = coeffs[i];
		for (j = i; j > 0; j--)
			coeffs[j] = coeffs[j-1] ^ rsProd(coeffs[j], rsalog[i]);
		coeffs[0] = rsProd(coeffs[0], rsalog[i]);
	}

}


// Perform Reed Solomon ECC codeword calculation
static void rsEncode(const uint8_t* datcws, const int datlen, uint8_t* ecccws, const int ecclen, const uint8_t* coeffs) {

	int i, j;
	uint8_t tmp[MAX_QR_DAT_CWS_PER_BLK + MAX_QR_ECC_CWS_PER_BLK] = { 0 };

	assert(datlen <= MAX_QR_DAT_CWS_PER_BLK);
	assert(ecclen <= MAX_QR_ECC_CWS_PER_BLK);

	memcpy(tmp, datcws, (size_t)datlen);

	for (i = 0; i < datlen; i++)
		for (j = 0; j < ecclen; j++)
			tmp[i+j+1] = rsProd(coeffs[ecclen-j-1], tmp[i]) ^ tmp[i+j+1];

	memcpy(ecccws, tmp + datlen, (size_t)ecclen);

}


// Plot all of the fixed-position artifacts and reserve space for the format
// and version information
static void plotFixtures(uint8_t *mtx, uint8_t *fix, const struct metric *m) {

	int i, j;

	// Plot timing patterns
	for (i = sizeof(finder[0]); i <= (int)(m->size - sizeof(finder[0]) - 1); i++) {
		putFixtureModule(i+1, 7, (uint8_t)(i+1)%2);
		putFixtureModule(7, i+1, (uint8_t)(i+1)%2);
	}

	// Plot finder patterns
	for (i = 0; i < (int)sizeof(finder[0]); i++) {
		for (j = 0; j < (int)sizeof(finder[0]); j++) {
			putFixtureModule( i+1,  j+1, finder[i][j]);
			putFixtureModule(-i-1,  j+1, finder[i][j]);
			putFixtureModule( i+1, -j-1, finder[i][j]);
		}
	}

	// Plot alignment patterns
	for (i = m->align[0] - 2; i <= m->size - 13; i += m->align[1] - m->align[0]) {
		putAlign(i+1, 5);
		putAlign(5, i+1);
	}
	for (i = m->align[0] - 2; i <= m->size - 9; i += m->align[1] - m->align[0])
		for (j = m->align[0] - 2; j <= m->size - 9; j += m->align[1] - m->align[0])
			putAlign(i+1, j+1);

	// Reserve the format information modules
	for (i = 0; i < (int)(SIZEOF_ARRAY(formatpos)); i++) {
		putFixtureModule(formatpos[i][0][0], formatpos[i][0][1], 1);
		putFixtureModule(formatpos[i][1][0], formatpos[i][1][1], 1);
	}

	// Reserve the version information modules
	if (m->size >= 45) {
		for (i = 0; i < (int)(SIZEOF_ARRAY(versionpos)); i++) {
			putFixtureModule(versionpos[i][0][0], versionpos[i][0][1], 0);
			putFixtureModule(versionpos[i][1][0], versionpos[i][1][1], 0);
		}
	}

	// Reserve the solitary dark module
	putFixtureModule(9, -8, 0);

}


// Apply a mask function to the non-fixture modules of a matrix
static void applyMask(uint8_t *dest, const uint8_t *src,
		      uint8_t (*maskfun)(const uint8_t x, const uint8_t y), const uint8_t *fix, const struct metric *m) {

	int i, j;

	for (i = 1; i <= m->size; i++) {
		for (j = 1; j <= m->size; j++) {
			putModule(dest, i, j, (uint8_t)(getModule(src, i, j) ^ (
				(~getModule(fix, i, j)) &
				((*maskfun) ((uint8_t)(i-1), (uint8_t)(j-1))))
			));
		}
	}

}


/*
 *  Calculate the n1 + n3 score for a runlength encoded row or column
 *
 *  rle is zero-terminated and will begin with a non-terminating 0 in the case
 *  of a row or columns starting with a dark module.
 *
 */
static int evaln1n3(const uint8_t *rle) {

	const uint8_t *s = rle;
	int n1 = 0, n3 = 0;
	int i, len;

	// Detect runs of 5 or more like modules
	do {
		if (*rle >= 5) n1 += *rle - 2;
	} while (*++rle);
	len = (int)(rle -s);
	rle = s;

	// Detect 1:1:3:1:1 ratio next to 4 modules of whitespace
	for (i = 3; i <= len - 3; i += 2)
		if (rle[i] % 3 == 0					&&	// Multiple of 3 black modules
			(rle[i-2] == rle[i-1] && rle[i-1] == rle[i+1]	&&	// in the ratio 1:1:3:1:1
			 rle[i+1] == rle[i+2] && rle[i+2] == rle[i]/3)	&&
				(
					(i == 3 || (i + 4 >= len)) || 		// at either extent of run
					(rle[i-3] >= 4 || rle[i+3] >= 4)	// or bounded by dark modules
				)
		   )
			n3 += 40;

	return n1 + n3;
}


// Evaluate a mask returning its total score
static uint32_t evalMask(const uint8_t *mtx, const struct metric *m) {

	int i, j, k, p;
	uint8_t size = m->size;
	uint8_t pairsa[MAX_QR_SIZE] = { 0 }, pairsb[MAX_QR_SIZE] = { 0 };
	uint8_t rlec[MAX_QR_SIZE] = { 0 }, rler[MAX_QR_SIZE] = { 0 };
	uint8_t lastc, lastr, qc, qr;
	int now, last;
	int n1n3 = 0, n2 = 0, n4 = 0;
	uint8_t *lastpairs = &pairsa[0], *thispairs = &pairsb[0], *tmppairs;

	assert(size >= 21);  // Satisfy static analyser (div by 0)

	for (k = 1; k <= m->size; k++) {

		/*
		 *  Runlength encode a column and row to enable efficient
		 *  calculation of the n1 and n3 scores.
		 *
		 *  Generates {0, ...} when starting with dark module
		 *
		 */
		qc = lastc = getModule(mtx, k, 1);
		rlec[0] = lastc ^ 1;
		rlec[1] = 1;
		qr = lastr = getModule(mtx, 1, k);
		rler[0] = lastr ^ 1;
		rler[1] = 1;
		for (p = 2; p <= size; p++) {
			if (getModule(mtx, k, p) == lastc) {
				rlec[qc]++;
			}
			else {
				rlec[++qc] = 1;
				lastc ^= 1;
			}
			if (getModule(mtx, p, k) == lastr) {
				rler[qr]++;
			}
			else {
				rler[++qr] = 1;
				lastr ^= 1;
			}
		}
		rlec[++qc] = rler[++qr] = 0;
		n1n3 += evaln1n3(rlec);
		n1n3 += evaln1n3(rler);

		// Score same coloured blocks
		tmppairs = lastpairs;
		lastpairs = thispairs;
		thispairs = tmppairs;
		last = getModule(mtx, 1, k) ^ 1;
		for (i = 1; i <= size; i++) {
			now = getModule(mtx, i, k);
			thispairs[i-1] = (uint8_t)(now + last);
			last = now;
		}
		if (k > 1)
			for (i = size-1; i>=0; i--)
				if (((thispairs[i] + lastpairs[i]) & 3) == 0)
					n2 += 3;
	}

	// Score dark/light balance
	for (i = 1; i <= size; i++)
		for (j = 1; j <= size; j++)
			n4 += getModule(mtx, i, j) == 1;
	n4 = abs(n4*100/(size*size)-50)/5*10;

	return (uint32_t)(n1n3+n2+n4);
}


// Append bits to a byte-encoded sequence
static void addBits(uint8_t bitField[], uint16_t* bitPos, int length, uint16_t bits, const int max_length, const bool truncate) {
	int i;

	if (length == 0 || *bitPos == UINT16_MAX)
		return;

	assert(length > 0 && length <= (int)(sizeof(bits)*8));
	assert(*bitPos <= max_length);

	if (!truncate && (*bitPos + length > max_length)) {
		*bitPos = UINT16_MAX;   // Indicate bust
		return;
	}

	// Truncate is enabled so clamp to maximum length
	if (*bitPos + length > max_length)
		length = max_length - *bitPos;

	for (i = length-1; i >= 0; i--) {
		if ((bits & 1) != 0) {
			bitField[(*bitPos+i)/8] = (uint8_t)(bitField[(*bitPos+i)/8] | (0x80 >> ((*bitPos+i)%8)));
		}
		else {
			bitField[(*bitPos+i)/8] = (uint8_t)(bitField[(*bitPos+i)/8] & (~(0x80 >> ((*bitPos+i)%8))));
		}
		bits >>= 1;
	}
	*bitPos = (uint16_t)(*bitPos + length);

	return;
}


// Generate the bitstream that represents the data message as a sequence of 8-bit codewords and length
static void createCodewords(gs1_encoder *ctx, const uint8_t *str, uint8_t cws_v[3][MAX_QR_CWS], uint16_t bits_v[3]) {

	int i;
	bool gs1Mode = false;
	const uint8_t *p;

	(void) ctx;		// Silence compiler

	if (*str == '^') {		// "^..." => GS1 mode
		gs1Mode = true;
		str++;
	} else {
		// Unescape leading sequence "\\...^" -> "\...^"
		p = str;
		while (*p == '\\')
			p++;
		if (*p == '^')
			str++;
	}

	/*
	 * Elements of the encoded message have differing lengths based on the
	 * resulting symbol size. The symbol sizes with different element
	 * lengths are batched into vergrps. To later pick the smallest symbol
	 * that holds our content we encode the message according to each
	 * available vergrp, based on the format of symbol.
	 *
	 */
	for (i = 0; i < 3; i++) {

		p = str;

		// 0101 FNC1 in first
		if (gs1Mode)
			addBits(cws_v[i], &bits_v[i], 4, 0x05, MAX_QR_DAT_BITS, false);

		// 0100 Enter byte mode
		addBits(cws_v[i], &bits_v[i], 4, 0x04, MAX_QR_DAT_BITS, false);

		// Character count indicator
		addBits(cws_v[i], &bits_v[i], cclens[i][2],
			(uint16_t)strlen((char *)p), MAX_QR_DAT_BITS, false);

		// Byte per character
		while (*p) {
			if (*p == '^' && gs1Mode)
				addBits(cws_v[i], &bits_v[i], 8, 0x1d, MAX_QR_DAT_BITS, false);  // FNC1 -> GS
			else
				addBits(cws_v[i], &bits_v[i], 8, *p, MAX_QR_DAT_BITS, false);
			p++;
		}

	}

}


// Select a symbol version that is sufficent to hold the encoded bitstream
static const struct metric* selectVersion(gs1_encoder *ctx, const uint16_t bits_v[3]) {

	const struct metric *m = NULL;
	int vers, ncws, ecws, dcws, dmod;
	uint16_t bits;
	bool okay;

	// Select a suitable symbol
	for (vers = 1; vers < (int)(SIZEOF_ARRAY(metrics)); vers++) {
		m = &metrics[vers];
		ncws = m->modules/8;				// Total number of codewords
		ecws = m->ecc_cws[ctx->qrEClevel -
			gs1_encoder_qrEClevelL];		// Number of error correction codewords
		dcws = ncws - ecws;				// Number of data codeword
		dmod = dcws*8;					// Number of data modules
		okay = true;
		bits = bits_v[m->vergrp];
		if (ctx->qrVersion != 0 &&
		    ctx->qrVersion != vers) okay = false; 	// User specified version
		if (bits > dmod) okay = false;			// Bitstream must fit capacity of symbol
		if (okay) break;
	}

	return okay ? m : NULL;

}


// Add terminator and padding to the bitstream then perform Reed Solomon Error Correction
static void finaliseCodewords(gs1_encoder *ctx, uint8_t *cws, uint16_t *bits, const struct metric *m) {

	uint8_t tmpcws[MAX_QR_CWS];

	uint8_t coeffs[MAX_QR_ECC_CWS_PER_BLK+1];

	int ncws, rbit, ecws, dcws, dmod, ecb1, ecb2, dcpb, ecpb;

	uint8_t *p;
	int i, j, a, b;

	ncws = m->modules/8;				// Total number of codewords
	rbit = m->modules%8;				// Number of remainder bit
	ecws = m->ecc_cws[ctx->qrEClevel -
		gs1_encoder_qrEClevelL];		// Number of error correction codewords
	dcws = ncws - ecws;				// Number of data codeword
	dmod = dcws*8;					// Number of data modules
	ecb1 = m->ecc_blks[ctx->qrEClevel -
		gs1_encoder_qrEClevelL][0];		// First error correction blocks
	ecb2 = m->ecc_blks[ctx->qrEClevel -
		gs1_encoder_qrEClevelL][1];		// First error correction blocks
	dcpb = dcws/(ecb1+ecb2);			// Base data codewords per block
	ecpb = ncws/(ecb1+ecb2) - dcpb;			// Error correction codewords per block

	assert(dcpb <= MAX_QR_DAT_CWS_PER_BLK);
	assert(ecpb <= MAX_QR_ECC_CWS_PER_BLK);

	// Complete the message bits by adding the terminator, truncated if neccessary
	addBits(cws, bits, 4, 0x00, dmod, true);  // 0000, or shorter at end

	// Expand the message bits by adding padding as necessary
	while (*bits < dmod) {
		addBits(cws, bits, 8, 0xEC, dmod, true);   // 11101100
		addBits(cws, bits, 8, 0x11, dmod, true);   // 00010001
	}
	assert(*bits == dmod);

	// Generate coefficients
	rsGenerateCoeffs(ecpb, coeffs);

	// Calculate the error correction codewords in two groups of blocks
	memcpy(tmpcws, cws, (size_t)dcws);
	for (i = 0; i < ecb1; i++)
		rsEncode(cws + i*dcpb, dcpb, tmpcws + dcws + i*ecpb, ecpb, coeffs);
	for (i = 0; i < ecb2; i++)
		rsEncode(cws + ecb1*dcpb + i*(dcpb+1), dcpb + 1,
			 tmpcws + dcws + (i+ecb1)*ecpb, ecpb, coeffs);

	// Reassemble the codewords by interleaving the data and ECC blocks
	p = cws;
	for (i = 0; i < dcpb+1; i++)
		for (j = 0; j < ecb1 + ecb2; j++)
			if ( i < dcpb || j >= ecb1) {  // First block group is shorter
				a = j < ecb1 ? j : ecb1;
				b = j-a;
				*p++ = tmpcws[a*dcpb + b*(dcpb+1) + i];
			}
	for (i = 0; i < ecpb; i++)
		for (j = 0; j < ecb1 + ecb2; j++)
			*p++ = tmpcws[dcws + j*ecpb + i];

	*bits = (uint16_t)(ncws*8);

	// Extend codewords by one if there are remainder bits
	if (rbit != 0)
		cws[ncws++] = 0;

}


// Create a symbol that holds the given bitstream
static void createMatrix(gs1_encoder *ctx, uint8_t *mtx, const uint8_t *cws, const struct metric *m) {

	uint8_t fix[MAX_QR_BYTES] = { 0 };	// Matrix in which 1 indicates fixed pattern
	uint8_t msk[MAX_QR_BYTES];		// Matrix used for mask evaluation

	uint8_t mask = 0;			// Satisfy compiler
	uint32_t formatval, versionval;
	uint32_t bestScore = UINT32_MAX, score;

	int i, j, k, col, dir;

	// Plot fixtures, including reservation of format and version
	// information
	plotFixtures(mtx, fix, m);

	// Walk the symbol placing the bitstream avoiding fixed patterns
	i = j = m->size;
	dir = -1;   // -1 updates; 1 downwards
	col = 1;    // 0 is left bit; 1 is right bit
	for (k = 0; i >= 1; )
	{
		if (!getModule(fix, i, j)) {
			putModule(mtx, i, j, (uint8_t)((cws[k/8] >> (7-k%8)) & 1));
			k++;
		}
		if (col == 1) {
			col = 0;
			i--;
			continue;
		}
		col = 1;
		i++;
		j += dir;
		if (j >= 1 && j <= m->size)
			continue;
		// Turn around at top and bottom
		dir *= -1;
		i -= 2;
		j += dir;
		if (i == 7)  // Hop over the timing pattern
			i--;
	}
	assert(k == m->modules);  // Filled the symbol

	// Evaluate the masked symbols to find the most suitable
	for (k = 0; k < (int)(SIZEOF_ARRAY(maskfun)); k++) {
		applyMask(msk, mtx, maskfun[k], fix, m);
		score = evalMask(msk, m);
		if (score < bestScore) {
			mask = (uint8_t)k;
			bestScore = score;
		}
	}
	applyMask(mtx, mtx, maskfun[mask], fix, m);

	// Set the solitary dark module
	putModule(mtx, 9, -8, 1);

	// Plot the format information
	switch (ctx->qrEClevel) {
		case gs1_encoder_qrEClevelL: formatval = formatmap[ 8 + mask]; break;
		case gs1_encoder_qrEClevelM: formatval = formatmap[ 0 + mask]; break;
		case gs1_encoder_qrEClevelQ: formatval = formatmap[24 + mask]; break;
		case gs1_encoder_qrEClevelH: formatval = formatmap[16 + mask]; break;
		default:
			assert(true);
			return;
	}
	for (i = 0; i < (int)(SIZEOF_ARRAY(formatpos)); i++) {
		putModule(mtx, formatpos[i][0][0], formatpos[i][0][1], (uint8_t)((formatval >> (14-i)) & 1));
		putModule(mtx, formatpos[i][1][0], formatpos[i][1][1], (uint8_t)((formatval >> (14-i)) & 1));
	}

	// Plot the version information modules
	if (m->size >= 45) {
		versionval = versionmap[(m->size-17)/4-7];
		for (i = 0; i < (int)(SIZEOF_ARRAY(versionpos)); i++) {
			putModule(mtx, versionpos[i][0][0], versionpos[i][0][1], (uint8_t)((versionval >> (17-i)) & 1));
			putModule(mtx, versionpos[i][1][0], versionpos[i][1][1], (uint8_t)((versionval >> (17-i)) & 1));
		}
	}

}


static int QRenc(gs1_encoder *ctx, const uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_QR_BYTES] = { 0 };
	uint8_t cws_v[3][MAX_QR_CWS] = { 0 };	// vergrp specific encodings
	uint16_t bits_v[3] = { 0 };
	const struct metric *m;

	assert(ctx->qrEClevel >= gs1_encoder_qrEClevelL && ctx->qrEClevel <= gs1_encoder_qrEClevelH);
	assert(ctx->qrVersion >= 0 && ctx->qrVersion <= 40);

	DEBUG_PRINT("\nData: %s\n", string);

	if (*string == '^' && strchr((char *)string, '|') != NULL) {
		strcpy(ctx->errMsg, "Composite component is not supported for QR Code");
		ctx->errFlag = true;
		return 0;
	}

	// For GS1 purposes we restrict to AI or DL only
	if (!(*string == '^' ||
	     (strlen((char*)string) >= 8 && strncmp((char*)string, "https://", 8) == 0) ||
	     (strlen((char*)string) >= 7 && strncmp((char*)string, "http://",  7) == 0)) ) {
		strcpy(ctx->errMsg, "QR Code input must be either an AI element string or a Digital Link URI");
		ctx->errFlag = true;
		return 0;
	}

	createCodewords(ctx, string, cws_v, bits_v);
	if (bits_v[0] == UINT16_MAX && bits_v[1] == UINT16_MAX && bits_v[2] == UINT16_MAX) {
		strcpy(ctx->errMsg, "Data exceeds the capacity of any QR Code symbol");
		ctx->errFlag = true;
		return 0;
	}

	assert(bits_v[0] <= MAX_QR_DAT_BITS || bits_v[0] == UINT16_MAX);
	assert(bits_v[1] <= MAX_QR_DAT_BITS || bits_v[1] == UINT16_MAX);
	assert(bits_v[2] <= MAX_QR_DAT_BITS || bits_v[2] == UINT16_MAX);

	if (bits_v[0] != UINT16_MAX)
		DEBUG_PRINT_BITS("Bitstream (vergrp 1)", cws_v[0], bits_v[0]);
	if (bits_v[1] != UINT16_MAX)
		DEBUG_PRINT_BITS("Bitstream (vergrp 2)", cws_v[1], bits_v[1]);
	if (bits_v[2] != UINT16_MAX)
		DEBUG_PRINT_BITS("Bitstream (vergrp 3)", cws_v[2], bits_v[2]);

	m = selectVersion(ctx, bits_v);
	if (!m) {
		strcpy(ctx->errMsg, "Data exceeds the capacity of the specified symbol");
		ctx->errFlag = true;
		return 0;
	}

	DEBUG_PRINT("Symbol: V%d-%d (cws: %d; ecc: %d; blocks: %d+%d)\n",
		m->version, ctx->qrEClevel, 99,
		m->ecc_cws[ctx->qrEClevel - gs1_encoder_qrEClevelL],
		m->ecc_blks[ctx->qrEClevel - gs1_encoder_qrEClevelL][0],
		m->ecc_blks[ctx->qrEClevel - gs1_encoder_qrEClevelL][1]);

	DEBUG_PRINT_CWS("Codewords", cws_v[m->vergrp], (uint16_t)((bits_v[m->vergrp]-1)/8+1));

	finaliseCodewords(ctx, cws_v[m->vergrp], &bits_v[m->vergrp], m);

	assert(bits_v[m->vergrp] <= MAX_QR_CWS*8);

	DEBUG_PRINT_CWS("Final codewords", cws_v[m->vergrp], bits_v[m->vergrp]/8);

	createMatrix(ctx, mtx, cws_v[m->vergrp], m);

	DEBUG_PRINT_MATRIX("Matrix", mtx, m->size + 2*QR_QZ, m->size + 2*QR_QZ);

	gs1_mtxToPatterns(mtx, m->size + 2*QR_QZ, m->size + 2*QR_QZ, pats);

	DEBUG_PRINT_PATTERN_LENGTHS("Patterns", pats, m->size + 2*QR_QZ);

	return m->size + 2*QR_QZ;

}


void gs1_QR(gs1_encoder *ctx) {

	struct sPrints prints = { 0 };
	struct patternLength *pats;
	char* dataStr = ctx->dataStr;
	int rows, cols, i;

	pats = ctx->qr_pats;

	if (!(rows = QRenc(ctx, (uint8_t*)dataStr, pats)) || ctx->errFlag)
		goto out;

	cols = 0;
	for (i = 0; i < pats[0].length; i++)
		cols += pats[0].pattern[i];

	gs1_driverInit(ctx, (long)ctx->pixMult*cols, (long)ctx->pixMult*rows);

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


void test_qr_QR_fixtures(void) {

	int v, i, j, cnt;
	const struct metric *m;
	uint8_t mtx[MAX_QR_BYTES];
	uint8_t fix[MAX_QR_BYTES];
	char casename[4];

	// Check that the modules available after plotting the fixtures matches
	// the values provided by the specification
	for (v = 1; v <= 40; v++) {
		m = &(metrics[v]);
		memset(mtx, 0, MAX_QR_BYTES);
		memset(fix, 0, MAX_QR_BYTES);
		plotFixtures(mtx, fix, m);
		for (i = 1, cnt = 0; i <= m->size; i++)
			for (j = 1; j <= m->size; j++)
				cnt += getModule(fix, i, j) ^ 1;
		sprintf(casename, "V%d", v);
		TEST_CASE(casename);
		TEST_CHECK(cnt == m->modules);
		TEST_MSG("Expected %d; Got %d", m->modules, cnt);
	}

}


void test_qr_QR_versions(void) {

	int v, ec;
	char casename[6];

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	gs1_encoder_setFormat(ctx, gs1_encoder_dRAW);
	gs1_encoder_setSym(ctx, gs1_encoder_sQR);
	gs1_encoder_setDataStr(ctx, "^99A");

	for (v = 1; v <= 40; v++) {
		for (ec = gs1_encoder_qrEClevelL; ec <= gs1_encoder_qrEClevelH; ec++) {
			gs1_encoder_setQrVersion(ctx, v);
			gs1_encoder_setQrEClevel(ctx, ec);
			sprintf(casename, "V%d-%d", v, ec);
			TEST_CASE(casename);
			TEST_CHECK(gs1_encoder_encode(ctx));
		}
	}

	gs1_encoder_free(ctx);

}

void test_qr_QR_encode(void) {

	const char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (const char*[]){
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"    XXXXXXX X  XXX X   X  XXXXXXX    ",
"    X     X X X XX  XX    X     X    ",
"    X XXX X        XX X X X XXX X    ",
"    X XXX X X X   XXXXXX  X XXX X    ",
"    X XXX X   XXX XX XXXX X XXX X    ",
"    X     X   X  X   X XX X     X    ",
"    XXXXXXX X X X X X X X XXXXXXX    ",
"            X   X  X X  X            ",
"    X XX XXX  X  X   X XX X  X XX    ",
"    X   XX  X XXXXX    X  XXX   X    ",
"        X X   XX X  X   X   X XX     ",
"     X X    XXXXX  XX XX X X    X    ",
"         XX    X  XXXXX XX   XX      ",
"    XX  XX XX X X XX XXXXXX   XXX    ",
"    X XXXXXX    XX  X  XX X X XXX    ",
"     X XXX XX X   X    XX  X   X     ",
"    X     XX XXXX X X  XX  XXX X     ",
"      XXXX XXX XX XXX X      XXX     ",
"    X X  XXX     XXXXXX X X X X      ",
"       X    XX   XX  X X  X   X      ",
"     X XX XXX X  XX  X XXXXXXXX      ",
"            XX XX       X   XXXXX    ",
"    XXXXXXX XXX   X X  XX X XX X     ",
"    X     X X  X XXXX X X   XX  X    ",
"    X XXX X   XXX  XXXX XXXXX X      ",
"    X XXX X XX  X  X XX  X XXX  X    ",
"    X XXX X X XX X  X  X   X  X X    ",
"    X     X  X XX X    XX XXXX X     ",
"    XXXXXXX XXXX X  X  XXXXXX  X     ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
NULL
	};
	TEST_CHECK(test_encode(ctx, true, gs1_encoder_sQR, "https://id.gs1.org/01/12312312312333", expect));

	expect = (const char*[]){
"                                     ",
"                                     ",
"                                     ",
"                                     ",
"    XXXXXXX     XX  X     XXXXXXX    ",
"    X     X  X XX X XXX X X     X    ",
"    X XXX X XX  X  X   X  X XXX X    ",
"    X XXX X XXXXX  XXXX X X XXX X    ",
"    X XXX X X     XX  XX  X XXX X    ",
"    X     X X X    X XX   X     X    ",
"    XXXXXXX X X X X X X X XXXXXXX    ",
"            XXXX XXXX   X            ",
"    X XXXXX  XXXXXX  XX   XXXXX      ",
"      X X  X  X XX  X  X X  X        ",
"      XX XX XX X  X   X  X X XXX     ",
"    X X  X X X     X X  X X X   X    ",
"      XXX XX XXXX  X    XX XX XX     ",
"     X X X   XX X XX                 ",
"     XXX XX  X XX  XXXX  X X XXX     ",
"        XX   XX  XX     X X X   X    ",
"     XX X X X X XXXXX X  X  XXXX     ",
"    X       X XXXX       XX    X     ",
"    X X  XXXX  X  XXXXX XX X XXX     ",
"    X XXX  X  X        X  X XX  X    ",
"    X  XXXXXX XX   X  X XXXXX  X     ",
"            XX XX X X  XX   X        ",
"    XXXXXXX  XXXX  XXXX X X XXXX     ",
"    X     X X   XXX     X   XX XX    ",
"    X XXX X XXXXX XX XX XXXXXX X     ",
"    X XXX X X   XX X  XXXX X   X     ",
"    X XXX X X XXX XXXXX   XXXXX      ",
"    X     X   X X X    X X X X       ",
"    XXXXXXX XX     XX X   X X X      ",
"                                     ",
"                                     ",
"                                     ",
"                                     ",
NULL
	};
	TEST_CHECK(test_encode(ctx, true, gs1_encoder_sQR, "^011234567890123110ABC123^11210630", expect));  // GS1 mode


	TEST_CHECK(test_encode(ctx, false, gs1_encoder_sQR, "^0112345678901231|^99ABC", NULL));  // CC is invalid


	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
