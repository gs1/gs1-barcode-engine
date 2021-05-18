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

#include <stdint.h>

#include "mtx.h"


void gs1_mtxPutBit(uint8_t *mtx, int cols, int x, int y, uint8_t bit) {

	int p = ((cols-1)/8+1)*y + x/8;

	if (bit != 0) {
		mtx[p] = (uint8_t)(mtx[p] | (0x80 >> x%8));
	}
	else {
		mtx[p] = (uint8_t)(mtx[p] & (~(0x80 >> x%8)));
	}

}


uint8_t gs1_mtxGetBit(uint8_t *mtx, int cols, int x, int y) {

	int p = ((cols-1)/8+1)*y + x/8;

	return (mtx[p] & (0x80 >> x%8)) != 0 ? 1:0;

}


// Runlength encode the matrix to a set of patterns
void gs1_mtxToPatterns(uint8_t* mtx, int cols, int rows, struct patternLength *pats) {

	uint8_t r, c, patPos, last;

	for (r = 0; r < rows; r++) {
		patPos = 0;
		last = gs1_mtxGetBit(mtx, cols, 0, r);
		pats[r].whtFirst = (last == 0);
		pats[r].pattern[0] = 1;
		for (c = 1; c < cols; c++) {
			if (gs1_mtxGetBit(mtx, cols, c, r) == last) {
				pats[r].pattern[patPos]++;
			}
			else {
				pats[r].pattern[++patPos] = 1;
				last ^= 1;
			}
		}
		pats[r].length = (uint8_t)(patPos+1);
	}

}
