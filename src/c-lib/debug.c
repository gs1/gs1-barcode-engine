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
#include <stdio.h>

#include "mtx.h"


#if PRNT

void do_debug_print_cws(char *prefix, uint8_t* cws, uint16_t cwslen) {

	int i;

	printf("%s: ", prefix);
	for (i = 0; i < cwslen; i++) {
		printf("%d ", cws[i]);
	}
	printf("\n");
}

void do_debug_print_pattern_lengths(char* prefix, struct patternLength *pats, int rows) {

	int i, j;

	printf("\n%s:\n", prefix);
	for (i = 0; i < rows; i++) {
		printf("%s:", pats[i].whtFirst ? "W" : "B");
		for (j = 0; j < pats[i].length; j++) {
			printf("%d", pats[i].pattern[j]);
		}
		printf("\n");
	}
	printf("\n");

}

#endif
