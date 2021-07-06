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

#ifndef AI_H
#define AI_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#define MAX_AIS		64
#define MAX_AI_LEN	90


typedef enum {
	cset_none = 0,
	cset_X,
	cset_N,
	cset_C,
} cset_t;


struct aiEntry;		// Must forward declare

typedef bool (*linter_t)(gs1_encoder *ctx, const struct aiEntry *entry, char *val);


// A single AI may consist of multiple concatenated components
struct aiComponent {
	cset_t cset;
	uint8_t min;
	uint8_t max;
	linter_t linters[1];
};


struct aiEntry {
	char *ai;
	bool fnc1;
	struct aiComponent parts[5];
	const char *title;
};

struct aiValue {
	const struct aiEntry *aiEntry;
	const char *ai;
	uint8_t ailen;
	const char *value;
	uint8_t vallen;
};


// Write to unbracketed AI dataStr checking for overflow
#define writeDataStr(v) do {						\
	if (strlen(dataStr) + strlen(v) > MAX_DATA)			\
		goto fail;						\
	strcat(dataStr, v);						\
} while (0)

#define nwriteDataStr(v,l) do {						\
	if (strlen(dataStr) + l > MAX_DATA)				\
		goto fail;						\
	strncat(dataStr, v, l);						\
} while (0)


#include "gs1encoders.h"

const struct aiEntry* gs1_lookupAIentry(gs1_encoder *ctx, const char *p, size_t ailen);
bool gs1_isFNC1required(const char *ai);
uint8_t gs1_aiLengthByPrefix(const char *ai);
bool gs1_aiValLengthContentCheck(gs1_encoder *ctx, const struct aiEntry *entry, const char *aiVal, size_t vallen);
bool gs1_parseAIdata(gs1_encoder *ctx, const char *aiData, char *dataStr);
bool gs1_processAIdata(gs1_encoder *ctx, const char *dataStr, bool extractAIs);
bool gs1_validateParity(uint8_t *str);
bool gs1_allDigits(const uint8_t *str, size_t len);


#ifdef UNIT_TESTS

void test_ai_lookupAIentry(void);
void test_ai_AItableVsPrefixLength(void);
void test_ai_parseAIdata(void);
void test_ai_processAIdata(void);
void test_ai_validateParity(void);

#endif


#endif  /* AI_H */
