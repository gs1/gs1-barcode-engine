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

#include <stdint.h>
#include <stdio.h>

#include "debug.h"
#include "mtx.h"


#if PRNT


void do_debug_print_cws(const char *prefix, const uint8_t* cws, const uint16_t cwslen) {

	int i;

	printf("%s: ", prefix);
	for (i = 0; i < cwslen; i++) {
		printf("%d ", cws[i]);
	}
	printf("\n");
}


void do_debug_print_bits(const char *prefix, const uint8_t *bits, const int numbits) {

	int i;

	printf("%s: ", prefix);
	for (i = 0; i < numbits; i++) {
		printf("%d", bits[i/8] >> (7-i%8) & 1);
	}
	printf("\n");

}


void do_debug_print_matrix(const char* prefix, const uint8_t *mtx, const int c, const int r) {

	int x, y;
	int bw = (c-1)/8+1;

	printf("%s:\n  ", prefix);
	for (y = 0; y < r; y++) {
		for (x = 0; x < c; x++) {
			printf("%s", (mtx[bw*y + x/8] >> (7-x%8) & 1) == 1 ? "X" : ".");
		}
		printf("\n  ");
	}
	printf("\n");

}


void do_debug_print_pattern_lengths(const char* prefix, const struct patternLength *pats, const int rows) {

	int i, j;

	printf("%s:\n  ", prefix);
	for (i = 0; i < rows; i++) {
		printf("%s:", pats[i].whtFirst ? "W" : "B");
		for (j = 0; j < pats[i].length; j++) {
			printf("%d ", pats[i].pattern[j]);
		}
		printf("\n  ");
	}
	printf("\n");

}


void do_debug_print_pattern(const char *prefix, const uint8_t* pattern, const int elements) {

	int i;

	printf("%s:\n  ", prefix);
	for (i = 0; i < elements; i++) {
		printf("%d ", pattern[i]);
	}
	printf("\n");

}


void do_debug_print_patterns(const char *prefix, const uint8_t* patterns, const int elements, const int rows) {

	int i, j;

	printf("%s:\n  ", prefix);
	for (i = 0; i < rows; i++) {
		for (j = 0; j < elements; j++) {
			printf("%d ", *(patterns+i*elements+j));
		}
		printf("\n  ");
	}
	printf("\n");

}


#endif
