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

#ifndef CC_H
#define CC_H

#include <stdint.h>

#include "enc-private.h"

#define CCB2_WIDTH 57		// 2 column cca/b
#define CCB2_ELMNTS 31		// includes qz's

#define CCA3_WIDTH 74		// 3 column cca
#define CCA3_ELMNTS 39		// includes qz's
#define MAX_CCA3_ROWS 8		// cca-3 max rows

#define CCB3_WIDTH 84		// 3 column ccb
#define CCB3_ELMNTS 45		// includes qz's

#define CCB4_WIDTH 101		// 4 column cca/b
#define CCB4_ELMNTS 53		// includes qz's
#define MAX_CCB4_ROWS 44	// ccb-4 max rows

uint8_t ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

int CC2enc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CC3enc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CC4enc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
bool CCCenc(gs1_encoder *ctx, uint8_t str[], uint8_t pattern[]);

int check2DData(uint8_t dataStr[]);
int pack(gs1_encoder *ctx, uint8_t str[], uint8_t bitField[]);
void putBits(uint8_t bitField[], int bitPos, int length, uint16_t bits);

#endif /* CC_H */
