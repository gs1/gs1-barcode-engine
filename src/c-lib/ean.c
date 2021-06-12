/**
 * GS1 barcode encoder library
 *
 * @author Copyright (c) 2000-2021 GS1 AISBL.
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
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "enc-private.h"
#include "cc.h"
#include "debug.h"
#include "driver.h"
#include "ean.h"
#include "gs1.h"


#define EAN13_ELMNTS	61	// includes qz's
#define EAN13_W		109 	// includes 7X quiet zones
#define EAN13_H		74	// total ht in x

#define EAN13_L_PAD	3	// EAN-13 7-X qz - CCA-2 3 offset
#define EAN13_R_PAD	5	// EAN-13 WIDTH - MAX_WIDTH - EAN13_L_PAD


// call with str = 13-digit primary
static bool EAN13enc(uint8_t *str, uint8_t pattern[] ) {

	static const uint16_t upcTblA[10] = {	0x3211, 0x2221, 0x2122, 0x1411, 0x1132,
					0x1231, 0x1114, 0x1312, 0x1213, 0x3112 };
	static const uint16_t upcTblB[10] = {	0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
					0x1321, 0x4111, 0x2131, 0x3121, 0x2113 };
	static const uint16_t abArr[10] = { 0 /*0x07*/,0x0B,0x0D,0x0E,0x13,0x19,0x1C,0x15,0x16,0x1A };
	static const uint8_t lGuard[4] = { 7,1,1,1 };
	static const uint8_t center[5] = { 1,1,1,1,1 };
	static const uint8_t rGuard[4] = { 1,1,1,7 };

	int i, j, abMask, bars, sNdx, pNdx, abBits;

	assert(str && strlen((char*)str) == 13);
	assert(gs1_allDigits(str));
	assert(gs1_validateParity(str));

	sNdx = 1;
	pNdx = 0;
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = lGuard[i];
	}
	abBits = abArr[str[0]-'0'];
	for (abMask = 0x20, i = 0; i < 6; abMask >>= 1, i++) {
		if ((abBits & abMask) == 0) {
			bars = upcTblA[str[sNdx++]-'0'];
		}
		else {
			bars = upcTblB[str[sNdx++]-'0'];
		}
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (uint8_t)((bars >> j) & 0xf);
		}
	}
	for (i = 0; i < 5; i++) {
		pattern[pNdx++] = center[i];
	}
	for (i = 0; i < 6; i++) {
		bars = upcTblA[str[sNdx++]-'0'];
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (uint8_t)((bars >> j) & 0xf);
		}
	}
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = rGuard[i];
	}
	return(true);
}

bool gs1_normaliseEAN13(gs1_encoder *ctx, char *dataStr, char *primaryStr) {

	unsigned int digits = ctx->sym == gs1_encoder_sEAN13 ? 13 : 12;

	if (strlen(dataStr) >= 17-digits && strncmp(dataStr, "#0100", 17-digits) == 0)
		dataStr += 17-digits;

	if (!ctx->addCheckDigit) {
		if (strlen(dataStr) != digits) {
			sprintf(ctx->errMsg, "primary data must be %d digits", digits);
			ctx->errFlag = true;
			*primaryStr = '\0';
			return false;
		}
	}
	else {
		if (strlen(dataStr) != digits-1) {
			sprintf(ctx->errMsg, "primary data must be %d digits without check digit", digits-1);
			ctx->errFlag = true;
			*primaryStr = '\0';
			return false;
		}
	}

	if (!gs1_allDigits((uint8_t*)dataStr)) {
		strcpy(ctx->errMsg, "primary data must be all digits");
		ctx->errFlag = true;
			*primaryStr = '\0';
		return false;
	}

	primaryStr[0] = ctx->sym == gs1_encoder_sEAN13 ? '\0' : '0';  // Convert GTIN-12 to GTIN-13 if UPC-A
	primaryStr[1] = '\0';
	strcat(primaryStr, dataStr);

	if (ctx->addCheckDigit)
		strcat(primaryStr, "-");

	if (!gs1_validateParity((uint8_t*)primaryStr) && !ctx->addCheckDigit) {
		strcpy(ctx->errMsg, "primary data check digit is incorrect");
		ctx->errFlag = true;
			*primaryStr = '\0';
		return false;
	}

	return true;

}

void gs1_EAN13(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints sepPrnt;

	uint8_t linPattern[EAN13_ELMNTS];
	uint8_t sepPat1[5] = { 7,1,EAN13_W-16,1,7 }; // separator pattern 1
	uint8_t sepPat2[5] = { 6,1,EAN13_W-14,1,6 }; // separator pattern 2

	char *dataStr = ctx->dataStr;
	char primaryStr[14];

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	int i;
	int rows, ccFlag;
	char *ccStr;

	DEBUG_PRINT("\nData: %s\n", dataStr);

	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
		DEBUG_PRINT("Primary %s\n", dataStr);
		DEBUG_PRINT("CC: %s\n", ccStr);
	}

	if (!gs1_normaliseEAN13(ctx, dataStr, primaryStr))
		goto out;

	DEBUG_PRINT("Checked: %s\n", primaryStr);

	if (!EAN13enc((uint8_t*)primaryStr, linPattern) || ctx->errFlag) goto out;

	DEBUG_PRINT_PATTERN("Linear pattern", linPattern, EAN13_ELMNTS);

	ctx->line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = EAN13_ELMNTS;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = ctx->pixMult*EAN13_H;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	// init most likely separator values
	sepPrnt.elmCnt = 5;
	sepPrnt.pattern = sepPat1;
	sepPrnt.guards = false;
	sepPrnt.height = ctx->pixMult*2;
	sepPrnt.leftPad = 0;
	sepPrnt.rightPad = 0;
	sepPrnt.whtFirst = true;
	sepPrnt.reverse = false;

	if (ccFlag) {
		if (!((rows = gs1_CC4enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) goto out;

		DEBUG_PRINT_PATTERNS("CC pattern", (uint8_t*)(*ccPattern), CCB4_ELMNTS, rows);

		gs1_driverInit(ctx, ctx->pixMult*EAN13_W, ctx->pixMult*(rows*2 + 6 + EAN13_H));

		// Composite Component
		prints.elmCnt = CCB4_ELMNTS;
		prints.height = ctx->pixMult*2;
		prints.leftPad = EAN13_L_PAD;
		prints.rightPad = EAN13_R_PAD;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator
		gs1_driverAddRow(ctx, &sepPrnt);
		sepPrnt.pattern = sepPat2;
		gs1_driverAddRow(ctx, &sepPrnt);
		sepPrnt.pattern = sepPat1;
		gs1_driverAddRow(ctx, &sepPrnt);

		// EAN-13
		prints.elmCnt = EAN13_ELMNTS;
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*EAN13_H;
		prints.leftPad = 0;
		prints.rightPad = 0;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);

	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*EAN13_W, ctx->pixMult*EAN13_H);

		// EAN-13
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}

out:

	// Restore the original dataStr contents
	if (ccFlag)
		*(ccStr-1) = '|';

	return;
}


#define EAN8_ELMNTS	45	// includes qz's
#define EAN8_W		81	// includes 7X quiet zones
#define EAN8_H		60	// total ht in x

#define EAN8_L_PAD	2	// EAN-8 7-X qz - CCA-2 offset
#define EAN8_R_PAD	5	// EAN-8 WIDTH - MAX_WIDTH of cca - EAN8_L_PAD
#define EAN8_L_PADB	8	// EAN-8 left pad for ccb

// call with str = 8-digit primary with check digit = 0
static bool EAN8enc(uint8_t str[], uint8_t pattern[] ) {

	static const uint16_t upcTblA[10] = {	0x3211, 0x2221, 0x2122, 0x1411, 0x1132,
					0x1231, 0x1114, 0x1312, 0x1213, 0x3112 };
	static const uint8_t lGuard[4] = { 7,1,1,1 };
	static const uint8_t center[5] = { 1,1,1,1,1 };
	static const uint8_t rGuard[4] = { 1,1,1,7 };

	int i, j, bars, sNdx, pNdx;

	assert(str && strlen((char*)str) == 8);
	assert(gs1_allDigits(str));
	assert(gs1_validateParity(str));

	sNdx = 0;
	pNdx = 0;
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = lGuard[i];
	}
	for (i = 0; i < 4; i++) {
		bars = upcTblA[str[sNdx++]-'0'];
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (uint8_t)((bars >> j) & 0xf);
		}
	}
	for (i = 0; i < 5; i++) {
		pattern[pNdx++] = center[i];
	}
	for (i = 0; i < 4; i++) {
		bars = upcTblA[str[sNdx++]-'0'];
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (uint8_t)((bars >> j) & 0xf);
		}
	}
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = rGuard[i];
	}
	return(true);
}

bool gs1_normaliseEAN8(gs1_encoder *ctx, char* dataStr, char* primaryStr) {

	if (strlen(dataStr) >= 9 && strncmp(dataStr, "#01000000", 9) == 0)
		dataStr += 9;

	if (!ctx->addCheckDigit) {
		if (strlen(dataStr) != 8) {
			strcpy(ctx->errMsg, "primary data must be 8 digits");
			ctx->errFlag = true;
			*primaryStr = '\0';
			return false;
		}
	}
	else {
		if (strlen(dataStr) != 7) {
			strcpy(ctx->errMsg, "primary data must be 7 digits without check digit");
			ctx->errFlag = true;
			*primaryStr = '\0';
			return false;
		}
	}

	if (!gs1_allDigits((uint8_t*)dataStr)) {
		strcpy(ctx->errMsg, "primary data must be all digits");
		ctx->errFlag = true;
		*primaryStr = '\0';
		return false;
	}

	strcpy(primaryStr, dataStr);

	if (ctx->addCheckDigit)
		strcat(primaryStr, "-");

	if (!gs1_validateParity((uint8_t*)primaryStr) && !ctx->addCheckDigit) {
		strcpy(ctx->errMsg, "primary data check digit is incorrect");
		ctx->errFlag = true;
		*primaryStr = '\0';
		return false;
	}

	return true;

}

void gs1_EAN8(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints sepPrnt;

	uint8_t linPattern[EAN8_ELMNTS];
	uint8_t sepPat1[5] = { 7,1,EAN8_W-16,1,7 }; // separator pattern 1
	uint8_t sepPat2[5] = { 6,1,EAN8_W-14,1,6 }; // separator pattern 2

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	char *dataStr = ctx->dataStr;
	char primaryStr[8+1];

	int i;
	int rows, ccFlag;
	char *ccStr;
	int lpadCC;
	int lpadEAN;
	int elmntsCC;

	DEBUG_PRINT("\nData: %s\n", dataStr);

	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
		DEBUG_PRINT("Primary %s\n", dataStr);
		DEBUG_PRINT("CC: %s\n", ccStr);
	}

	if (!gs1_normaliseEAN8(ctx, dataStr, primaryStr))
		goto out;

	DEBUG_PRINT("Checked: %s\n", primaryStr);

	if (!EAN8enc((uint8_t*)primaryStr, linPattern) || ctx->errFlag) goto out;

	DEBUG_PRINT_PATTERN("Linear pattern", linPattern, EAN8_ELMNTS);

	ctx->line1 = true; // so first line is not Y undercut
	// init most likely prints values
	lpadEAN = 0;
	lpadCC = EAN8_L_PAD;
	elmntsCC = CCA3_ELMNTS;
	prints.elmCnt = EAN8_ELMNTS;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = ctx->pixMult*EAN8_H;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	// init most likely separator values
	sepPrnt.elmCnt = 5;
	sepPrnt.pattern = sepPat1;
	sepPrnt.guards = false;
	sepPrnt.height = ctx->pixMult*2;
	sepPrnt.leftPad = 0;
	sepPrnt.rightPad = 0;
	sepPrnt.whtFirst = true;
	sepPrnt.reverse = false;
	if (ccFlag) {
		if (!((rows = gs1_CC3enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) goto out;
		if (rows > MAX_CCA3_ROWS) { // CCB composite
			lpadEAN = EAN8_L_PADB;
			lpadCC = 0;
			elmntsCC = CCB3_ELMNTS;
			prints.leftPad = EAN8_L_PADB;
			sepPrnt.leftPad = EAN8_L_PADB;
		}

		DEBUG_PRINT_PATTERNS("CC pattern", (uint8_t*)(*ccPattern), elmntsCC, rows);

		gs1_driverInit(ctx, ctx->pixMult*(EAN8_W+lpadEAN), ctx->pixMult*(rows*2 + 6 + EAN8_H));

		// Composite Component
		prints.elmCnt = elmntsCC;
		prints.height = ctx->pixMult*2;
		prints.leftPad = lpadCC;
		prints.rightPad = EAN8_R_PAD;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator
		gs1_driverAddRow(ctx, &sepPrnt);
		sepPrnt.pattern = sepPat2;
		gs1_driverAddRow(ctx, &sepPrnt);
		sepPrnt.pattern = sepPat1;
		gs1_driverAddRow(ctx, &sepPrnt);

		// EAN-8
		prints.elmCnt = EAN8_ELMNTS;
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*EAN8_H;
		prints.leftPad = lpadEAN;
		prints.rightPad = 0;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*EAN8_W, ctx->pixMult*EAN8_H);

		// EAN-8
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}

out:

	// Restore the original dataStr contents
	if (ccFlag)
		*(ccStr-1) = '|';

	return;
}


#define UPCE_ELMNTS	35	// includes qz's
#define UPCE_W		65	// includes 7X quiet zones
#define UPCE_H		74	// total ht in x

#define UPCE_L_PAD	3	// UPC-E 7X qz - 4X
#define UPCE_R_PAD	5	// UPCE_W - MAX_WIDTH - UPCE_L_PAD

// call with str = 7-digit primary
static bool UPCEenc(uint8_t str[], uint8_t pattern[] ) {

	static const uint16_t upcTblA[10] = {	0x3211, 0x2221, 0x2122, 0x1411, 0x1132,
					0x1231, 0x1114, 0x1312, 0x1213, 0x3112 };
	static const uint16_t upcTblB[10] = {	0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
					0x1321, 0x4111, 0x2131, 0x3121, 0x2113 };
	static const uint16_t abArr[10] = { 0x07,0x0B,0x0D,0x0E,0x13,0x19,0x1C,0x15,0x16,0x1A };
	static const uint8_t lGuard[4] = { 7,1,1,1 };
	static const uint8_t rGuard[7] = { 1,1,1,1,1,1,7 };

	int i, j, abMask, bars, sNdx, pNdx, abBits;

	assert(str && strlen((char*)str) == 7);
	assert(gs1_allDigits(str));

	sNdx = 0;
	pNdx = 0;
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = lGuard[i];
	}
	abBits = abArr[str[6]-'0'];
	for (abMask = 0x20, i = 0; i < 6; abMask >>= 1, i++) {
		if ((abBits & abMask) != 0) {
			bars = upcTblA[str[sNdx++]-'0'];
		}
		else {
			bars = upcTblB[str[sNdx++]-'0'];
		}
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (uint8_t)((bars >> j) & 0xf);
		}
	}
	for (i = 0; i < 7; i++) {
		pattern[pNdx++] = rGuard[i];
	}
	return(true);
}


/*
 *  Zero-compression for GTIN-12
 *
 *  abcdeNX <=> 0abN0000cdeX  :  0<N<2
 *  abcde3X <=> 0abc00000deX
 *  abcde4X <=> 0abcd00000eX
 *  abcdeNX <=> 0abcde0000NX  :  5<N<9
 *
 */
static bool zeroCompress(char *primaryStr, char *data7) {

	int i;

	assert(primaryStr && strlen((char*)primaryStr) == 12);
	assert(data7);

	if (primaryStr[0] != '0') {
		return false;
	}
	for (i = 0; i < 5; i++) {
		data7[i] = primaryStr[i+1];
	}
	if (primaryStr[3] >= '0' && primaryStr[3] <= '2' &&
			primaryStr[4] == '0' && primaryStr[5] == '0' && primaryStr[6] == '0' && primaryStr[7] == '0') {
		// 0abc0000hij = abhijc, where c = 0-2
		data7[2] = primaryStr[8];
		data7[3] = primaryStr[9];
		data7[4] = primaryStr[10];
		data7[5] = primaryStr[3];
	}
	else if (primaryStr[4] == '0' && primaryStr[5] == '0' && primaryStr[6] == '0' &&
			primaryStr[7] == '0' && primaryStr[8] == '0') {
		// 0abc00000ij = abcij3
		data7[3] = primaryStr[9];
		data7[4] = primaryStr[10];
		data7[5] = '3';
	}
	else if (primaryStr[5] == '0' && primaryStr[6] == '0' &&
			primaryStr[7] == '0' && primaryStr[8] == '0' && primaryStr[9] == '0') {
		// 0abcd00000j = abcdj4
		data7[4] = primaryStr[10];
		data7[5] = '4';
	}
	else if (primaryStr[10] >= '5' && primaryStr[10] <= '9' && primaryStr[6] == '0' &&
			primaryStr[7] == '0' && primaryStr[8] == '0' && primaryStr[9] == '0') {
		// 0abcde0000j = abcdej where j = 5-9
		data7[5] = primaryStr[10];
	}
	else {
		return false;
	}
	data7[6] = primaryStr[11];
	data7[7] = '\0';

	return true;

}

bool gs1_normaliseUPCE(gs1_encoder *ctx, char *dataStr, char *primaryStr) {

	if (strlen(dataStr) >= 5 && strncmp(dataStr, "#0100", 5) == 0)
		dataStr += 5;

	if (!ctx->addCheckDigit) {
		if (strlen(dataStr) != 12) {
			strcpy(ctx->errMsg, "primary data must be 12 digits");
			ctx->errFlag = true;
			*primaryStr = '\0';
			return false;
		}
	}
	else {
		if (strlen(dataStr) != 11) {
			strcpy(ctx->errMsg, "primary data must be 11 digits without check digit");
			ctx->errFlag = true;
			*primaryStr = '\0';
			return false;
		}
	}

	if (!gs1_allDigits((uint8_t*)dataStr)) {
		strcpy(ctx->errMsg, "primary data must be all digits");
		ctx->errFlag = true;
		*primaryStr = '\0';
		return false;
	}

	strcpy(primaryStr, dataStr);

	if (ctx->addCheckDigit)
		strcat(primaryStr, "-");

	if (!gs1_validateParity((uint8_t*)primaryStr) && !ctx->addCheckDigit) {
		strcpy(ctx->errMsg, "primary data check digit is incorrect");
		ctx->errFlag = true;
		*primaryStr = '\0';
		return false;
	}

	return true;

}

void gs1_UPCE(gs1_encoder *ctx) {

	struct sPrints prints;
	struct sPrints sepPrnt;

	uint8_t linPattern[UPCE_ELMNTS];
	uint8_t sepPat1[5] = { 7,1,UPCE_W-16,1,7 }; // separator pattern 1
	uint8_t sepPat2[5] = { 6,1,UPCE_W-14,1,6 }; // separator pattern 2

	uint8_t (*ccPattern)[CCB4_ELMNTS] = ctx->ccPattern;

	char *dataStr = ctx->dataStr;
	char primaryStr[12+1];
	char data7[7+1];

	int i;
	int rows, ccFlag;
	char *ccStr;

	DEBUG_PRINT("\nData: %s\n", dataStr);

	ccStr = strchr(dataStr, '|');
	if (ccStr == NULL) ccFlag = false;
	else {
		ccFlag = true;
		ccStr[0] = '\0'; // separate primary data
		ccStr++; // point to secondary data
		DEBUG_PRINT("Primary %s\n", dataStr);
		DEBUG_PRINT("CC: %s\n", ccStr);
	}

	if (!gs1_normaliseUPCE(ctx, dataStr, primaryStr))
		goto out;

	DEBUG_PRINT("Checked: %s\n", primaryStr);

	// Perform zero-compression
	if (!zeroCompress(primaryStr, data7)) {
		strcpy(ctx->errMsg, "Data cannot be converted to UPC-E");
		ctx->errFlag = true;
		goto out;
	}
	DEBUG_PRINT("Zero-compressed: %s\n", data7);

	if (!UPCEenc((uint8_t*)data7, linPattern) || ctx->errFlag) goto out;

	DEBUG_PRINT_PATTERN("Linear pattern", linPattern, UPCE_ELMNTS);

	ctx->line1 = true; // so first line is not Y undercut
	// init most likely prints values
	prints.elmCnt = UPCE_ELMNTS;
	prints.pattern = linPattern;
	prints.guards = false;
	prints.height = ctx->pixMult*UPCE_H;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.whtFirst = true;
	prints.reverse = false;
	// init most likely separator values
	sepPrnt.elmCnt = 5;
	sepPrnt.pattern = sepPat1;
	sepPrnt.guards = false;
	sepPrnt.height = ctx->pixMult*2;
	sepPrnt.leftPad = 0;
	sepPrnt.rightPad = 0;
	sepPrnt.whtFirst = true;
	sepPrnt.reverse = false;
	if (ccFlag) {
		if (!((rows = gs1_CC2enc(ctx, (uint8_t*)ccStr, ccPattern)) > 0) || ctx->errFlag) goto out;

		DEBUG_PRINT_PATTERNS("CC pattern", (uint8_t*)(*ccPattern), CCB2_ELMNTS, rows);

		gs1_driverInit(ctx, ctx->pixMult*UPCE_W, ctx->pixMult*(rows*2 + 6 + UPCE_H));

		// Composite Component
		prints.elmCnt = CCB2_ELMNTS;
		prints.height = ctx->pixMult*2;
		prints.leftPad = UPCE_L_PAD;
		prints.rightPad = UPCE_R_PAD;
		for (i = 0; i < rows; i++) {
			prints.pattern = ccPattern[i];
			gs1_driverAddRow(ctx, &prints);
		}

		// CC separator
		gs1_driverAddRow(ctx, &sepPrnt);
		sepPrnt.pattern = sepPat2;
		gs1_driverAddRow(ctx, &sepPrnt);
		sepPrnt.pattern = sepPat1;
		gs1_driverAddRow(ctx, &sepPrnt);

		// UPC-E
		prints.elmCnt = UPCE_ELMNTS;
		prints.pattern = linPattern;
		prints.height = ctx->pixMult*UPCE_H;
		prints.leftPad = 0;
		prints.rightPad = 0;
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}
	else { // primary only
		gs1_driverInit(ctx, ctx->pixMult*UPCE_W, ctx->pixMult*UPCE_H);

		// UPC-E
		gs1_driverAddRow(ctx, &prints);

		gs1_driverFinalise(ctx);
	}

out:

	// Restore the original dataStr contents
	if (ccFlag)
		*(ccStr-1) = '|';

	return;
}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_ean_EAN13_encode_ean13(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (char*[]){
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
"       X X  XX  X  XX  X  XX XX X    X X   XX XXX  X X X X X    X   X  X  X   XXX X  XXX  X XXX  X X X       ",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sEAN13, "#0102112345678900", expect));
	TEST_CHECK(test_encode(ctx, gs1_encoder_sEAN13, "2112345678900", expect));
	gs1_encoder_setAddCheckDigit(ctx, true);
	TEST_CHECK(test_encode(ctx, gs1_encoder_sEAN13, "211234567890", expect));

	gs1_encoder_free(ctx);

}


void test_ean_EAN13_encode_upca(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (char*[]){
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCA, "#0100416000336108", expect));
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCA, "416000336108", expect));
	gs1_encoder_setAddCheckDigit(ctx, true);
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCA, "41600033610", expect));

	gs1_encoder_free(ctx);

}


void test_ean_EAN8_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (char*[]){
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
"       X X   XX X  X  XX XXXX X X   XX X X X  XXX X X    X   X  X    X X X       ",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sEAN8, "#0100000002345673", expect));
	TEST_CHECK(test_encode(ctx, gs1_encoder_sEAN8, "02345673", expect));
	gs1_encoder_setAddCheckDigit(ctx, true);
	TEST_CHECK(test_encode(ctx, gs1_encoder_sEAN8, "0234567", expect));

	gs1_encoder_free(ctx);

}


void test_ean_UPCA_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (char*[]){
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
"       X X X   XX  XX  X X XXXX   XX X   XX X   XX X X X X    X X    X X X    XX  XX XXX  X X  X   X X       ",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCA, "#0100416000336108", expect));
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCA, "416000336108", expect));
	gs1_encoder_setAddCheckDigit(ctx, true);
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCA, "41600033610", expect));

	gs1_encoder_free(ctx);

}


static void test_zeroCompress(char* wide, char* narrow, bool valid) {

	char data7[8];
	char casename[25];

	assert(wide && strlen(wide) == 12);
	assert(narrow && strlen(narrow) == 7);

	sprintf(casename, "%s <=> %s", wide, narrow);

	TEST_CASE(casename);
	TEST_ASSERT(zeroCompress(wide, data7) ^ !valid);
	TEST_CHECK(!valid || strcmp(data7, narrow) == 0);
	TEST_MSG("Wanted %s. Got %s.", narrow, data7);

}


void test_ean_zeroCompress(void) {

	// abcdeNX <=> 0abN0000cdeX  :  0<N<2
	test_zeroCompress("01200000567X","125670X", true );
	test_zeroCompress("01210000567X","125671X", true );
	test_zeroCompress("01220000567X","125672X", true );
	test_zeroCompress("01230000567X","XXXXXXX", false);
	test_zeroCompress("01240000567X","XXXXXXX", false);
	test_zeroCompress("01250000567X","XXXXXXX", false);
	test_zeroCompress("01260000567X","XXXXXXX", false);
	test_zeroCompress("01270000567X","XXXXXXX", false);
	test_zeroCompress("01280000567X","XXXXXXX", false);
	test_zeroCompress("01290000567X","XXXXXXX", false);
	test_zeroCompress("01201000567X","XXXXXXX", false);
	test_zeroCompress("01200100567X","XXXXXXX", false);
	test_zeroCompress("01200010567X","XXXXXXX", false);
	test_zeroCompress("01200001567X","XXXXXXX", false);

	// abcde3X <=> 0abc00000deX
	test_zeroCompress("01230000045X","123453X", true );
	test_zeroCompress("01231000045X","XXXXXXX", false);
	test_zeroCompress("01230100045X","XXXXXXX", false);
	test_zeroCompress("01230010045X","XXXXXXX", false);
	test_zeroCompress("01230001045X","XXXXXXX", false);
	test_zeroCompress("01230000145X","XXXXXXX", false);

	// abcde4X <=> 0abcd00000eX
	test_zeroCompress("01234000005X","123454X", true );
	test_zeroCompress("01234100004X","XXXXXXX", false);	// Avoid match in next category
	test_zeroCompress("01234010005X","XXXXXXX", false);
	test_zeroCompress("01234001005X","XXXXXXX", false);
	test_zeroCompress("01234000105X","XXXXXXX", false);
	test_zeroCompress("01234000015X","XXXXXXX", false);

	// abcdeNX <=> 0abcde0000NX
	test_zeroCompress("01234500000X","XXXXXXX", false);
	test_zeroCompress("01234500001X","XXXXXXX", false);
	test_zeroCompress("01234500002X","XXXXXXX", false);
	test_zeroCompress("01234500003X","XXXXXXX", false);
	test_zeroCompress("01234500004X","XXXXXXX", false);
	test_zeroCompress("01234500005X","123455X", true );
	test_zeroCompress("01234500006X","123456X", true );
	test_zeroCompress("01234500007X","123457X", true );
	test_zeroCompress("01234500008X","123458X", true );
	test_zeroCompress("01234500009X","123459X", true );

}


void test_ean_UPCE_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	expect = (char*[]){
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
"       X X X  XXX  XX  X  XX XX XXXX X  XXX X XX   X X X X       ",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCE, "#0100001234000057", expect));
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCE, "001234000057", expect));
	gs1_encoder_setAddCheckDigit(ctx, true);
	TEST_CHECK(test_encode(ctx, gs1_encoder_sUPCE, "00123400005", expect));

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */

