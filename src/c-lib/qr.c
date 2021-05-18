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


static int QRenc(gs1_encoder *ctx, uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_QR_BYTES] = { 0 };

	int rows = 20, cols = 20;

(void) ctx;
(void) string;

	for (int i = 0 ; i<rows; i++) {

	for (int j=0; j<cols; j++) {
		gs1_mtxPutBit(mtx, cols, j, i, (i+j)%2);
	}
	}



	gs1_mtxToPatterns(mtx, cols, rows, pats);

	return rows;
}


void gs1_QR(gs1_encoder *ctx) {

	struct sPrints prints;
	struct patternLength pats[MAX_QR_ROWS];
	char* dataStr = ctx->dataStr;
	int rows, cols, i;

	// TODO dataStr checks
/*
	if (strlen(dataStr) > 13) {
		strcpy(ctx->errMsg, "primary data exceeds 13 digits");
		ctx->errFlag = true;
		return;
	}
*/

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
