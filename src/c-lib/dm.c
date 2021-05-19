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
#include "dm.h"
#include "mtx.h"
#include "driver.h"


#define METRIC(r, c, rh, rv, cw, bl)	\
	{ .rows = r, .cols = c,			\
	  .regh = rh, .regv = rv,		\
	  .rscw = cw, .rsbl = bl,		\
	}


struct metric {
	uint8_t rows;
	uint8_t cols;
	uint8_t regh;
	uint8_t regv;
	uint16_t rscw;
	uint8_t rsbl;
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


static int DMenc(gs1_encoder *ctx, uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_DM_BYTES] = { 0 };

	const struct metric *m = NULL;

	(void)ctx;
	(void)string;
	(void)pats;



	m = &metrics[0];


	gs1_mtxToPatterns(mtx, m->cols, m->rows, pats);

	return m->rows;

}


void gs1_DM(gs1_encoder *ctx) {

	struct sPrints prints;
	struct patternLength pats[MAX_DM_ROWS];
	char* dataStr = ctx->dataStr;
	int rows, cols, i;

	if (!(rows = DMenc(ctx, (uint8_t*)dataStr, pats)) || ctx->errFlag) return;

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


void test_dm_DM_encode(void) {

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
	TEST_CHECK(test_encode(ctx, gs1_encoder_sDM, "1501234567890", expect));

	test_print_strings(ctx);

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
