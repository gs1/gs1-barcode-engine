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

#ifndef GS1_H
#define GS1_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_AIS	64


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
	char *title;
};

struct aiValue {
	const struct aiEntry *aiEntry;
	char *value;
	uint8_t vallen;
};


#include "gs1encoders.h"

bool gs1_parseAIdata(gs1_encoder *ctx, char *aiData, char *dataStr);
bool gs1_processAIdata(gs1_encoder *ctx, char *dataStr);
bool gs1_validateParity(uint8_t *str);
bool gs1_allDigits(uint8_t *str);


#ifdef UNIT_TESTS

void test_gs1_parseAIdata(void);
void test_gs1_processAIdata(void);
void test_gs1_validateParity(void);

#endif


#endif  /* GS1_H */
