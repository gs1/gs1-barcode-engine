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

#ifndef MTX_H
#define MTX_H

#include <stdbool.h>
#include <stdint.h>


#define MAX_MTX_BYTES	32568;


struct patternLength {
	bool whtFirst;
	uint8_t pattern[UINT8_MAX];  // Sufficient for MAX_QR_ELMNTS
	uint8_t length;
};

void gs1_mtxPutModule(uint8_t *mtx, int cols, int x, int y, uint8_t bit);
uint8_t gs1_mtxGetModule(uint8_t *mtx, int cols, int x, int y);
void gs1_mtxToPatterns(uint8_t* mtx, int w, int h, struct patternLength *pats);


#endif  /* MTX_H */
