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

#include <stdint.h>

#include "mtx.h"


void gs1_mtxPutModule(uint8_t *mtx, const int cols, const int x, const int y, const uint8_t bit) {

	int p = ((cols-1)/8+1)*y + x/8;

	if (bit != 0) {
		mtx[p] = (uint8_t)(mtx[p] | (0x80 >> x%8));
	}
	else {
		mtx[p] = (uint8_t)(mtx[p] & (~(0x80 >> x%8)));
	}

}


uint8_t gs1_mtxGetModule(const uint8_t *mtx, const int cols, const int x, const int y) {

	int p = ((cols-1)/8+1)*y + x/8;

	return (uint8_t)((mtx[p] >> (7-x%8)) & 1);

}


// Runlength encode the matrix to a set of patterns
void gs1_mtxToPatterns(const uint8_t* mtx, const int cols, const int rows, struct patternLength *pats) {

	uint8_t patPos, last;
	int r, c;

	for (r = 0; r < rows; r++) {
		patPos = 0;
		last = gs1_mtxGetModule(mtx, cols, 0, r);
		pats[r].whtFirst = (last == 0);
		pats[r].pattern[0] = 1;
		for (c = 1; c < cols; c++) {
			if (gs1_mtxGetModule(mtx, cols, c, r) == last) {
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
