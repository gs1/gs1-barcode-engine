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

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "enc-private.h"
#include "qr.h"
#include "mtx.h"
#include "driver.h"



#define METRIC(vg, s, a2, a3, m, el, em, eq, eh, l1, l2, m1, m2, q1, q2, h1, h2) {	\
	   .vergrp = vg,								\
	   .size = s,									\
	   .align2 = a2,								\
	   .align3 = a3,								\
	   .modules = m,								\
	   .ecc_cws = { el, em, eq, eh }, 						\
	   .ecc_blks = { {l1,l2}, {m1,m2}, {q1,q2}, {h1,h2} },				\
	}


struct metric {
	uint8_t vergrp;
	uint8_t size;
	uint8_t align2;
	uint8_t align3;
	uint16_t modules;
	uint16_t ecc_cws[4];
	uint8_t ecc_blks[4][2];
};


static const struct metric metrics[40] = {
	//   grp  size    align  modules       error codewords         error correction blocks
	//               A2  A3              L     M     Q     H   L1  L2  M1  M2  Q1  Q2  H1  H2
	METRIC(1,   21,  98, 99,     208,    7,   10,   13,   17,   1,  0,  1,  0,  1,  0,  1,  0),   // v1
	METRIC(1,   25,  18, 99,     359,   10,   16,   22,   28,   1,  0,  1,  0,  1,  0,  1,  0),   // v2
	METRIC(1,   29,  22, 99,     567,   15,   26,   36,   44,   1,  0,  1,  0,  2,  0,  2,  0),   // v3
	METRIC(1,   33,  26, 99,     807,   20,   36,   52,   64,   1,  0,  2,  0,  2,  0,  4,  0),   // v4
	METRIC(1,   37,  30, 99,    1079,   26,   48,   72,   88,   1,  0,  2,  0,  2,  2,  2,  2),   // v5
	METRIC(1,   41,  34, 99,    1383,   36,   64,   96,  112,   2,  0,  4,  0,  4,  0,  4,  0),   // v6
	METRIC(1,   45,  22, 38,    1568,   40,   72,  108,  130,   2,  0,  4,  0,  2,  4,  4,  1),   // v7
	METRIC(1,   49,  24, 42,    1936,   48,   88,  132,  156,   2,  0,  2,  2,  4,  2,  4,  2),   // v8
	METRIC(1,   53,  26, 46,    2336,   60,  110,  160,  192,   2,  0,  3,  2,  4,  4,  4,  4),   // v9
	METRIC(2,   57,  28, 50,    2768,   72,  130,  192,  224,   2,  2,  4,  1,  6,  2,  6,  2),   // v10
	METRIC(2,   61,  30, 54,    3232,   80,  150,  224,  264,   4,  0,  1,  4,  4,  4,  3,  8),   // v11
	METRIC(2,   65,  32, 58,    3728,   96,  176,  260,  308,   2,  2,  6,  2,  4,  6,  7,  4),   // v12
	METRIC(2,   69,  34, 62,    4256,  104,  198,  288,  352,   4,  0,  8,  1,  8,  4, 12,  4),   // v13
	METRIC(2,   73,  26, 46,    4651,  120,  216,  320,  384,   3,  1,  4,  5, 11,  5, 11,  5),   // v14
	METRIC(2,   77,  26, 48,    5243,  132,  240,  360,  432,   5,  1,  5,  5,  5,  7, 11,  7),   // v15
	METRIC(2,   81,  26, 50,    5867,  144,  280,  408,  480,   5,  1,  7,  3, 15,  2,  3, 13),   // v16
	METRIC(2,   85,  30, 54,    6523,  168,  308,  448,  532,   1,  5, 10,  1,  1, 15,  2, 17),   // v17
	METRIC(2,   89,  30, 56,    7211,  180,  338,  504,  588,   5,  1,  9,  4, 17,  1,  2, 19),   // v18
	METRIC(2,   93,  30, 58,    7931,  196,  364,  546,  650,   3,  4,  3, 11, 17,  4,  9, 16),   // v19
	METRIC(2,   97,  34, 62,    8683,  224,  416,  600,  700,   3,  5,  3, 13, 15,  5, 15, 10),   // v20
	METRIC(2,  101,  28, 50,    9252,  224,  442,  644,  750,   4,  4, 17,  0, 17,  6, 19,  6),   // v21
	METRIC(2,  105,  26, 50,   10068,  252,  476,  690,  816,   2,  7, 17,  0,  7, 16, 34,  0),   // v22
	METRIC(2,  109,  30, 54,   10916,  270,  504,  750,  900,   4,  5,  4, 14, 11, 14, 16, 14),   // v23
	METRIC(2,  113,  28, 54,   11796,  300,  560,  810,  960,   6,  4,  6, 14, 11, 16, 30,  2),   // v24
	METRIC(2,  117,  32, 58,   12708,  312,  588,  870, 1050,   8,  4,  8, 13,  7, 22, 22, 13),   // v25
	METRIC(2,  121,  30, 58,   13652,  336,  644,  952, 1110,  10,  2, 19,  4, 28,  6, 33,  4),   // v26
	METRIC(3,  125,  34, 62,   14628,  360,  700, 1020, 1200,   8,  4, 22,  3,  8, 26, 12, 28),   // v27
	METRIC(3,  129,  26, 50,   15371,  390,  728, 1050, 1260,   3, 10,  3, 23,  4, 31, 11, 31),   // v28
	METRIC(3,  133,  30, 54,   16411,  420,  784, 1140, 1350,   7,  7, 21,  7,  1, 37, 19, 26),   // v29
	METRIC(3,  137,  26, 52,   17483,  450,  812, 1200, 1440,   5, 10, 19, 10, 15, 25, 23, 25),   // v30
	METRIC(3,  141,  30, 56,   18587,  480,  868, 1290, 1530,  13,  3,  2, 29, 42,  1, 23, 28),   // v31
	METRIC(3,  145,  34, 60,   19723,  510,  924, 1350, 1620,  17,  0, 10, 23, 10, 35, 19, 35),   // v32
	METRIC(3,  149,  30, 58,   20891,  540,  980, 1440, 1710,  17,  1, 14, 21, 29, 19, 11, 46),   // v33
	METRIC(3,  153,  34, 62,   22091,  570, 1036, 1530, 1800,  13,  6, 14, 23, 44,  7, 59,  1),   // v34
	METRIC(3,  157,  30, 54,   23008,  570, 1064, 1590, 1890,  12,  7, 12, 26, 39, 14, 22, 41),   // v35
	METRIC(3,  161,  24, 50,   24272,  600, 1120, 1680, 1980,   6, 14,  6, 34, 46, 10,  2, 64),   // v36
	METRIC(3,  165,  28, 54,   25568,  630, 1204, 1770, 2100,  17,  4, 29, 14, 49, 10, 24, 46),   // v37
	METRIC(3,  169,  32, 58,   26896,  660, 1260, 1860, 2220,   4, 18, 13, 32, 48, 14, 42, 32),   // v38
	METRIC(3,  173,  26, 54,   28256,  720, 1316, 1950, 2310,  20,  4, 40,  7, 43, 22, 10, 67),   // v39
	METRIC(3,  177,  30, 58,   29648,  750, 1372, 2040, 2430,  19,  6, 18, 31, 34, 34, 20, 61),   // v40
};


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


static const int8_t formatmap[15][2][2] = {
	{ {  1, 9 }, {  9,-1 } },  { {  2, 9 }, {  9,-2 } },  { {  3, 9 }, {  9,-3 } },
	{ {  4, 9 }, {  9,-4 } },  { {  5, 9 }, {  9,-5 } },  { {  6, 9 }, {  9,-6 } },
	{ {  8, 9 }, {  9,-7 } },  { {  9, 9 }, { -8, 9 } },  { {  9, 8 }, { -7, 9 } },
	{ {  9, 6 }, { -6, 9 } },  { {  9, 5 }, { -5, 9 } },  { {  9, 4 }, { -4, 9 } },
	{ {  9, 3 }, { -3, 9 } },  { {  9, 2 }, { -2, 9 } },  { {  9, 1 }, { -1, 9 } },
};


static const uint16_t formatvals[32] = {
	0x5412, 0x5125, 0x5e7c, 0x5b4b, 0x45f9, 0x40ce, 0x4f97, 0x4aa0,
	0x77c4, 0x72f3, 0x7daa, 0x789d, 0x662f, 0x6318, 0x6c41, 0x6976,
	0x1689, 0x13be, 0x1ce7, 0x19d0, 0x0762, 0x0255, 0x0d0c, 0x083b,
	0x355f, 0x3068, 0x3f31, 0x3a06, 0x24b4, 0x2183, 0x2eda, 0x2bed,
};


static const int8_t versionmap[18][2][2] = {
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


static const uint32_t versionvals[34] = {
	0x07c94, 0x085bc, 0x09a99, 0x0a4d3, 0x0bbf6, 0x0c762, 0x0d847,
	0x0e60d, 0x0f928, 0x10b78, 0x1145d, 0x12a17, 0x13532, 0x149a6,
	0x15683, 0x168c9, 0x177ec, 0x18ec4, 0x191e1, 0x1afab, 0x1b08e,
	0x1cc1a, 0x1d33f, 0x1ed75, 0x1f250, 0x209d5, 0x216fd, 0x228ba,
	0x2379f, 0x24b0b, 0x2542e, 0x26a64, 0x27541, 0x28c69,
};


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


static inline uint8_t mask1(uint8_t i, uint8_t j) {
	return (i+j)%2 == 0;
}

static inline uint8_t mask2(uint8_t i, uint8_t j) {
	(void)i;
	return j%2 == 0;
}

static inline uint8_t mask3(uint8_t i, uint8_t j) {
	(void)j;
	return i%3 == 0;
}

static inline uint8_t mask4(uint8_t i, uint8_t j) {
	return (i+j)%3 == 0;
}

static inline uint8_t mask5(uint8_t i, uint8_t j) {
	return (j/2+i/3)%2 == 0;
}

static inline uint8_t mask6(uint8_t i, uint8_t j) {
	return (i*j)%2+(i*j)%3 == 0;
}

static inline uint8_t mask7(uint8_t i, uint8_t j) {
	return ((i*j)%2+(i*j)%3)%2 == 0;
}

static inline uint8_t mask8(uint8_t i, uint8_t j) {
	return ((i*j)%3+(i+j)%2)%2 == 0;
}

static uint8_t (*const maskfun[8]) (uint8_t x, uint8_t y) = {
	mask1, mask2, mask3, mask4, mask5, mask6, mask7, mask8
};


// Syntactic sugar to include qz offset and wrap around processing
#define putBit(d, x, y, b) do {							\
	assert( x != 0 && y != 0 && abs(x) <= m->size && abs(y) <= m->size );	\
	gs1_mtxPutBit(d, m->size + 2*QR_QZ,					\
		x > 0 ? x + QR_QZ - 1 : m->size + QR_QZ + x,			\
		y > 0 ? y + QR_QZ - 1 : m->size + QR_QZ + y,			\
		b);								\
} while(0)

#define getBit(s, x, y)								\
	gs1_mtxGetBit(s, m->size + 2*QR_QZ,					\
		x > 0 ? x + QR_QZ - 1 : m->size + QR_QZ + x,			\
		y > 0 ? y + QR_QZ - 1 : m->size + QR_QZ + y)

#define putFixtureBit(x, y, b) do {						\
	putBit(mtx, x, y, b);							\
	putBit(fix, x, y, 1);							\
} while(0)

#define putAlign(x, y) do {							\
	doPutAlign(mtx, fix, m, x, y);						\
} while(0)


static void doPutAlign(uint8_t *mtx, uint8_t *fix, const struct metric *m, int x, int y) {

	int i, j;

	for (i = 0; i < (int)sizeof(algnpat[0]); i++)
		for (j = 0; j < (int)sizeof(algnpat[0]); j++)
			putFixtureBit(x+i, y+j, algnpat[i][j]);

}


// Reed Solomon product in the field
static inline uint8_t rsprod(uint8_t a, uint8_t b) {
	return a && b ? rsalog[ (rslog[a] + rslog[b]) % 255 ] : 0;
}


// Generate Reed Solomon coefficients
static void rscoeffs(int size, uint8_t *coeffs) {

	int i, j;

	coeffs[0] = 1;
	for (i = 0; i < size; i++) {
		coeffs[i+1] = coeffs[i];
		for (j = i; j > 0; j--)
			coeffs[j] = coeffs[j-1] ^ rsprod(coeffs[j], rsalog[i]);
		coeffs[0] = rsprod(coeffs[0], rsalog[i]);
	}
}


static void rsencode(uint8_t* datcws, int datlen, uint8_t* ecccws, int ecclen, uint8_t* coeffs) {

	int i, j;
	uint8_t tmp[MAX_QR_DAT_CWS_PER_BLK + MAX_QR_ECC_CWS_PER_BLK] = { 0 };

	memcpy(tmp, datcws, (size_t)datlen);

	for (i = 0; i < datlen; i++)
		for (j = 0; j < ecclen; j++)
			tmp[i+j+1] = rsprod(coeffs[ecclen-j-1], tmp[i]) ^ tmp[i+j+1];

	memcpy(ecccws, tmp + datlen, (size_t)ecclen);

}


static void applyMask(uint8_t *dest, uint8_t *src,
		      uint8_t (*maskfun)(uint8_t x, uint8_t y), uint8_t *fix, const struct metric *m) {

	int i, j;

	for (i = 1; i <= m->size; i++) {
		for (j = 1; j <= m->size; j++) {
			putBit(dest, i, j, (uint8_t)(getBit(src, i, j) ^ (
				(!getBit(fix, i, j)) &
				((*maskfun) ((uint8_t)(i-1), (uint8_t)(j-1))))
			));
		}
	}

}

/*
 *  rle is a zero-terminated runlength encoding of an entire row or column.
 *
 *  rle will beging with a non-terminating 0 in the case of a row or columns
 *  starting with a dark module.
 *
 */
static int evaln1n3(uint8_t *rle) {

	uint8_t *s = rle;
	int n1 = 0, n3 = 0;
	int i, len;

	// Detect runs of 5 or more like modules
	do {
		if (*rle >= 5) n1 += *rle - 2;
	} while (*++rle);
	len = (int)(rle -s);
	rle = s;

	// Detect 1:1:3:1:1 ratio next to 4 modules of whitespace
	for (i = 3; i <= len - 3; i += 2) {
		if (rle[i] % 3 == 0					&&	// Multiple of 3 black modules
			(rle[i-2] == rle[i-1] && rle[i-1] == rle[i+1]	&&	// in the ratio 1:1:3:1:1
			 rle[i+1] == rle[i+2] && rle[i+2] == rle[i]/3)	&& (
			(i == 3 || (i + 4 >= len)) || 				// at either extent of run
			(rle[i-3] >= 4 || rle[i+3] >= 4)			// or bounded by dark modules
		    ) )
			n3 += 40;
	}

	return n1 + n3;
}


static uint32_t evalMask(uint8_t *mtx, const struct metric *m) {

	int i, j, k, p;
	uint8_t size = m->size;
	uint8_t pairsa[MAX_QR_SIZE], pairsb[MAX_QR_SIZE];
	uint8_t rlec[MAX_QR_SIZE], rler[MAX_QR_SIZE];
	uint8_t lastc, lastr, qc, qr;
	int now, last;
	int n1n3 = 0, n2 = 0, n4 = 0;
	uint8_t *lastpairs = &pairsa[0], *thispairs = &pairsb[0], *tmppairs;

	assert(size >= 21);  // Satisfy static analyser (div by 0)

	for (k = 1; k <= m->size; k++) {

		// RLE columns and rows, 0:..:.. when starting with dark module
		qc = lastc = getBit(mtx, k, 1);
		rlec[0] = lastc ^ 1;
		rlec[1] = 1;
		qr = lastr = getBit(mtx, 1, k);
		rler[0] = lastr ^ 1;
		rler[1] = 1;
		for (p = 2; p <= size; p++) {
			if (getBit(mtx, k, p) == lastc) {
				rlec[qc]++;
			}
			else {
				rlec[++qc] = 1;
				lastc ^= 1;
			}
			if (getBit(mtx, p, k) == lastr) {
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

		// Count and score same coloured blocks
		tmppairs = lastpairs;
		lastpairs = thispairs;
		thispairs = tmppairs;

		last = getBit(mtx, 1, k) ^ 1;
		for (i = 1; i <= size; i++) {
			now = getBit(mtx, i, k);
			thispairs[i-1] = (uint8_t)(now + last);
			last = now;
		}
		if (k > 1)
			for (int i = size-1; i>=0; i--)
				if (((thispairs[i] + lastpairs[i]) & 3) == 0)
					n2 += 3;
	}

	// Dark/light balance
	for (i = 1; i <= size; i++)
		for (j = 1; j <= size; j++)
			n4 += getBit(mtx, i, j) == 1;
	n4 = abs(n4*100/(size*size)-50)/5*10;

	return (uint32_t)(n1n3+n2+n4);
}

static int QRenc(gs1_encoder *ctx, uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_QR_BYTES] = { 0 };
	uint8_t fix[MAX_QR_BYTES] = { 0 };	// 1 indicates fixed pattern
	uint8_t tmp[MAX_QR_BYTES];		// Used for mask evaluation

//	uint8_t cws[MAX_QR_CWS];
	uint8_t tmpcws[MAX_QR_CWS];
	int eclevel = ctx->qrEClevel;

	uint8_t coeffs[MAX_QR_ECC_CWS_PER_BLK+1];

	int i, j, k, col, dir;
	uint8_t mask = 0;			// Satisfy compiler
	uint32_t formatval, versionval;
	uint32_t bestScore = UINT32_MAX, score;
	const struct metric *m = NULL;

	uint8_t bitstream[MAX_QR_DATA_BYTES];
	uint16_t bitpos = 0;

	assert(eclevel >= gs1_encoder_qrEClevelL && eclevel <= gs1_encoder_qrEClevelH);


(void)bitstream;
(void)bitpos;

(void) ctx;
(void) string;

	int ver = 10 -1;

	m = &(metrics[ver]);

	int ncws = m->modules/8;		// Total number of codewords
	int rbit = m->modules%8;		// Number of remainder bit
	int ecws = m->ecc_cws[eclevel];		// Number of error correction codewords
	int dcws = ncws - ecws;			// Number of data codeword
	int dmod = dcws*8;			// Number of data modules
	int ecb1 = m->ecc_blks[eclevel][0];	// First error correction blocks
	int ecb2 = m->ecc_blks[eclevel][1];	// First error correction blocks

	// After symbol selection
	int dcpb = dcws/(ecb1+ecb2);		// Base data codewords per block
	int ecpb = ncws/(ecb1+ecb2) - dcpb;	// Error correction codewords per block

	assert(dcpb <= MAX_QR_DAT_CWS_PER_BLK);
	assert(ecpb <= MAX_QR_ECC_CWS_PER_BLK);


(void)dmod;


	uint8_t cws[MAX_QR_CWS] = {16,8,30,217,196,64,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17,236,17};  // TODO dummy



	// Generate coefficients
	rscoeffs(ecpb, coeffs);

	// Calculate the error correction codewords in two groups of blocks
	memcpy(tmpcws, cws, (size_t)dcws);
	for (i = 0; i < ecb1; i++)
		rsencode(cws + i*dcpb, dcpb, tmpcws + dcws + i*ecpb, ecpb, coeffs);
	for (i = 0; i < ecb2; i++)
		rsencode(cws + ecb1*dcpb + i*(dcpb+1), dcpb + 1, tmpcws + dcws + (i+ecb1)*ecpb, ecpb, coeffs);

	// Reassemble the codewords by interleaving the data and ECC blocks
	uint8_t *p = &(cws[0]);
	for (i = 0; i < dcpb+1; i++)
		for (j = 0; j < ecb1 + ecb2; j++)
			if ( i < dcpb || j >= ecb1)	// First block group is shorter
				*p++ = tmpcws[j*dcpb + i];
	for (i = 0; i < ecpb; i++)
		for (j = 0; j < ecb1 + ecb2; j++)
			*p++ = tmpcws[dcws + j*ecpb + i];

	// Extend codewords by one if there are remainder bits
	if (rbit != 0)
		cws[ncws++] = 0;

/*
printf("\n");
for (int i = 0 ; i < ncws; i++ ) {
	printf("%d ", cws[i]);
}
printf("\n");
*/

	// Plot timing patterns
	for (i = sizeof(finder[0]); i <= (int)(m->size - sizeof(finder[0]) - 1); i++) {
		putFixtureBit(i+1, 7, (uint8_t)(i+1)%2);
		putFixtureBit(7, i+1, (uint8_t)(i+1)%2);
	}

	// Plot finder patterns
	for (i = 0; i < (int)sizeof(finder[0]); i++) {
		for (j = 0; j < (int)sizeof(finder[0]); j++) {
			putFixtureBit( i+1,  j+1, finder[i][j]);
			putFixtureBit(-i-1,  j+1, finder[i][j]);
			putFixtureBit( i+1, -j-1, finder[i][j]);
		}
	}

	// Plot alignment patterns
	for (i = m->align2 - 2; i <= m->size - 13; i += m->align3 - m->align2) {
		putAlign(i+1, 5);
		putAlign(5, i+1);
	}
	for (i = m->align2 - 2; i <= m->size - 9; i += m->align3 - m->align2)
		for (j = m->align2 - 2; j <= m->size - 9; j += m->align3 - m->align2)
			putAlign(i+1, j+1);

	// Reserve the format information modules
	for (i = 0; i < (int)(sizeof(formatmap) / sizeof(formatmap[0])); i++) {
		putFixtureBit(formatmap[i][0][0], formatmap[i][0][1], 1);
		putFixtureBit(formatmap[i][1][0], formatmap[i][1][1], 1);
	}

	// Reserve the version information modules
	if (m->size >= 45) {
		for (i = 0; i < (int)(sizeof(versionmap) / sizeof(versionmap[0])); i++) {
			putFixtureBit(versionmap[i][0][0], versionmap[i][0][1], 0);
			putFixtureBit(versionmap[i][1][0], versionmap[i][1][1], 0);
		}
	}

	// Reserve the solitary dark module
	putFixtureBit(9, -8, 0);

	// Walk the symbol placing the bitstream avoiding fixed patterns
	i = j = m->size;
	dir = -1;   // -1 updates; 1 downwards
	col = 1;    // 0 is left bit; 1 is right bit
	for (k = 0; i >= 1; )
	{
		if (!getBit(fix, i, j)) {
			putBit(mtx, i, j, (cws[k/8] & (0x80>>(k%8))) != 0);
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
	for (k = 0; k < (int)(sizeof(maskfun) / sizeof(maskfun[0])); k++) {
		applyMask(tmp, mtx, maskfun[k], fix, m);
		score = evalMask(tmp, m);
		if (score < bestScore) {
			mask = (uint8_t)k;
			bestScore = score;
		}
	}
	applyMask(mtx, mtx, maskfun[mask], fix, m);

	// Set the solitary dark module
	putBit(mtx, 9, -8, 1);

	// Plot the format information
	switch (ctx->qrEClevel) {
		case gs1_encoder_qrEClevelL: formatval = formatvals[ 8 + mask]; break;
		case gs1_encoder_qrEClevelM: formatval = formatvals[ 0 + mask]; break;
		case gs1_encoder_qrEClevelQ: formatval = formatvals[24 + mask]; break;
		case gs1_encoder_qrEClevelH: formatval = formatvals[16 + mask]; break;
		default:
			assert(true);
			return -1;  // Satisfy compiler
	}
	for (i = 0; i < (int)(sizeof(formatmap) / sizeof(formatmap[0])); i++) {
		putBit(mtx, formatmap[i][0][0], formatmap[i][0][1], (formatval >> (14-i)) & 1);
		putBit(mtx, formatmap[i][1][0], formatmap[i][1][1], (formatval >> (14-i)) & 1);
	}

	// Plot the version information modules
	versionval = versionvals[(m->size-17)/4-7];
	if (m->size >= 45) {
		for (i = 0; i < (int)(sizeof(versionmap) / sizeof(versionmap[0])); i++) {
			putBit(mtx, versionmap[i][0][0], versionmap[i][0][1], (versionval >> (17-i)) & 1);
			putBit(mtx, versionmap[i][1][0], versionmap[i][1][1], (versionval >> (17-i)) & 1);
		}
	}

	gs1_mtxToPatterns(mtx, m->size + 2*QR_QZ, m->size + 2*QR_QZ, pats);

	return m->size + 2*QR_QZ;
}


void gs1_QR(gs1_encoder *ctx) {

	struct sPrints prints;
	struct patternLength pats[MAX_QR_SIZE];
	char* dataStr = ctx->dataStr;
	int rows, cols, i;

	if (!(rows = QRenc(ctx, (uint8_t*)dataStr, pats)) || ctx->errFlag) return;

#if PRNT
	{
		int j;
		printf("\n%s\n", dataStr);
		printf("\n");
		for (i = 0; i < rows; i++) {
			for (j = 0; j < pats[i].length; j++) {
				printf("%d", pats[i].pattern[j]);
			}
			printf("\n");
		}
		printf("\n");
	}
#endif

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

	return;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_qr_QR_encode(void) {

//	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

/*
	expect = (char*[]){
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
NULL
	};
*/

        TEST_CHECK(gs1_encoder_setFormat(ctx, gs1_encoder_dRAW));
        TEST_CHECK(gs1_encoder_setSym(ctx, gs1_encoder_sQR));
        TEST_CHECK(gs1_encoder_setDataStr(ctx, "1501234567890"));
        TEST_CHECK(gs1_encoder_encode(ctx));

	test_print_strings(ctx);

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
