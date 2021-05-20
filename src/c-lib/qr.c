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

#include "enc-private.h"
#include "qr.h"
#include "mtx.h"
#include "driver.h"



#define METRIC(vg, s, a2, a3, m, el, em, eq, eh, l1, l2, m1, m2, q1, q2, h1, h2)	\
	{ .vergrp = vg, .size = s,							\
	   .align2 = a2, .align3 = a3, .modules = m,					\
	   .ecc_cws_l = el, .ecc_cws_m = em, .ecc_cws_q = eq, .ecc_cws_h = eh,		\
	   .ecc_blk_l1 = l1, .ecc_blk_l2 = l2,						\
	   .ecc_blk_m1 = m1, .ecc_blk_m2 = m2,						\
	   .ecc_blk_q1 = q1, .ecc_blk_q2 = q2,						\
	   .ecc_blk_h1 = h1, .ecc_blk_h2 = h2,						\
	}


struct metric {
	uint8_t vergrp;
	uint8_t size;
	uint8_t align2;
	uint8_t align3;
	uint16_t modules;
	uint16_t ecc_cws_l;
	uint16_t ecc_cws_m;
	uint16_t ecc_cws_q;
	uint16_t ecc_cws_h;
	uint8_t ecc_blk_l1;
	uint8_t ecc_blk_l2;
	uint8_t ecc_blk_m1;
	uint8_t ecc_blk_m2;
	uint8_t ecc_blk_q1;
	uint8_t ecc_blk_q2;
	uint8_t ecc_blk_h1;
	uint8_t ecc_blk_h2;
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
	 * Our coordinate system is indexed with (1,1) at top-left.  Negative
	 * cooridinates wrap around to the other extent of the same row/column,
	 * such that (-1,-1) is at bottom-right. The coordinate system is
	 * oblivious to the quiet zone.
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


// Syntactic sugar to include qz offset and wrap around processing
#define putBit(x, y, b) do {					\
	gs1_mtxPutBit(mtx, m->size + 2*QR_QZ,			\
		x > 0 ? x + QR_QZ - 1 : m->size + QR_QZ + x,	\
		y > 0 ? y + QR_QZ - 1 : m->size + QR_QZ + y,	\
		b);						\
} while(0)

#define putAlign(x, y) do {		\
	doPutAlign(mtx, m, x, y);	\
} while(0)

static void doPutAlign(uint8_t *mtx, const struct metric *m, int x, int y) {
	int i, j;
	for (i = 0; i < (int)sizeof(algnpat[0]); i++)
		for (j = 0; j < (int)sizeof(algnpat[0]); j++)
			putBit(x+i, y+j, algnpat[i][j]);
}


static int QRenc(gs1_encoder *ctx, uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_QR_BYTES] = { 0 };
	int i, j;

	uint8_t size = 177;

	const struct metric *m = NULL;

(void) ctx;
(void) string;

	// Lookup AP positions
	for (i = 0; i<40; i++) {
		m = &(metrics[i]);
		if (m->size == size)
			break;
	}

	// Plot timing patterns
	for (i = sizeof(finder[0]); i <= (int)(m->size - sizeof(finder[0]) - 1); i++) {
		putBit(i+1, 7, (uint8_t)(i+1)%2);
		putBit(7, i+1, (uint8_t)(i+1)%2);
	}

	// Plot finder patterns
	for (i = 0; i < (int)sizeof(finder[0]); i++) {
		for (j = 0; j < (int)sizeof(finder[0]); j++) {
			putBit(i+1, j+1, finder[i][j]);
			putBit(-i-1, j+1, finder[i][j]);
			putBit(i+1, -j-1, finder[i][j]);
		}
	}

	// Plot alignment patterns
	for (i = m->align2 - 2; i <= m->size - 13; i += m->align3 - m->align2) {
		putAlign(i+1, 5);
		putAlign(5, i+1);
	}
	for (i = m->align2 - 2; i <= m->size - 9; i += m->align3 - m->align2) {
		for (j = m->align2 - 2; j <= m->size - 9; j += m->align3 - m->align2) {
			putAlign(i+1, j+1);
		}
	}

	// Reserve the format information modules
	for (i = 0; i < (int)(sizeof(formatmap)/sizeof(formatmap[0])); i++) {
		putBit(formatmap[i][0][0], formatmap[i][0][1], 1);
		putBit(formatmap[i][1][0], formatmap[i][1][1], 1);
	}

	// Reserve the version information modules
	if (m->size >= 45) {
		for (i = 0; i < (int)(sizeof(versionmap)/sizeof(versionmap[0])); i++) {
			putBit(versionmap[i][0][0], versionmap[i][0][1], 1);
			putBit(versionmap[i][1][0], versionmap[i][1][1], 1);
		}
	}

	// Reserve the solitary dark module
	putBit(9, -8, 1);


(void)formatvals;
(void)versionvals;


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

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

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
	TEST_CHECK(test_encode(ctx, gs1_encoder_sQR, "1501234567890", expect));

	test_print_strings(ctx);

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
