/**
 * GS1 Barcode Engine
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

#ifndef CC_H
#define CC_H

#include <stdint.h>
#include <stdbool.h>


#define CCB2_WIDTH	57	// 2 column cca/b
#define CCB2_ELMNTS	31	// includes qz's

#define CCA3_WIDTH	74	// 3 column cca
#define CCA3_ELMNTS	39	// includes qz's
#define MAX_CCA3_ROWS	8	// cca-3 max rows

#define CCB3_WIDTH	84	// 3 column ccb
#define CCB3_ELMNTS	45	// includes qz's

#define CCB4_WIDTH	101	// 4 column cca/b
#define CCB4_ELMNTS	53	// includes qz's
#define MAX_CCB4_CW	176	// ccb-4 max codewords
#define MAX_CCB4_ROWS	44	// ccb-4 max rows
#define MAX_CCB4_BYTES	148	// maximum byte mode capacity for ccb4

#define MAX_CCC_CW	863	// ccc max data codewords
#define MAX_CCC_ROWS	90	// ccc max rows
#define MAX_CCC_BYTES	1033	// maximum byte mode capacity for ccc

#define MAX_CCA2_SIZE	6	// index to 167 in CC2Sizes
#define MAX_CCA3_SIZE	4	// index to 167 in CC3Sizes
#define MAX_CCA4_SIZE	4	// index to 197 in CC4Sizes


#include "enc-private.h"
#include "gs1encoders.h"

int gs1_CC2enc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int gs1_CC3enc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int gs1_CC4enc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
bool gs1_CCCenc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[]);

int gs1_check2DData(const uint8_t dataStr[]);
int gs1_pack(gs1_encoder *ctx, uint8_t str[], uint8_t bitField[]);
void gs1_putBits(gs1_encoder *ctx, uint8_t bitField[], int bitPos, int length, uint16_t bits);


#ifdef UNIT_TESTS

void test_cc_encode928(void);

#endif


#endif /* CC_H */
