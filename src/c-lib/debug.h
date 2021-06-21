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

#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

struct patternLength;


#if PRNT


#define DEBUG_PRINT(...) do {				\
	printf(__VA_ARGS__);				\
} while (0)


#define DEBUG_PRINT_CWS(p,c,l) do {			\
	do_debug_print_cws(p,c,l);			\
} while (0)


#define DEBUG_PRINT_BITS(p,b,l) do {			\
	do_debug_print_bits(p,b,l);			\
} while (0)


#define DEBUG_PRINT_MATRIX(p,m,c,r) do {		\
	do_debug_print_matrix(p,m,c,r);			\
} while (0)


#define DEBUG_PRINT_PATTERN_LENGTHS(m,p,r) do {		\
	do_debug_print_pattern_lengths(m,p,r);		\
} while (0)

#define DEBUG_PRINT_PATTERN(m,p,e) do {			\
	do_debug_print_pattern(m,p,e);			\
} while (0)

#define DEBUG_PRINT_PATTERNS(m,p,e,r) do {		\
	do_debug_print_patterns(m,p,e,r);		\
} while (0)


void do_debug_print_cws(const char *prefix, const uint8_t *cws, uint16_t cwslen);
void do_debug_print_bits(const char *prefix, const uint8_t *bits, int numbits);
void do_debug_print_matrix(const char *prefix, const uint8_t *mtx, int c, int r);
void do_debug_print_pattern_lengths(const char *prefix, const struct patternLength *pats, int rows);
void do_debug_print_pattern(const char *prefix, const uint8_t* pattern, int elements);
void do_debug_print_patterns(const char *prefix, const uint8_t* patterns, int elements, int rows);


#else

#define DEBUG_PRINT(...) {}
#define DEBUG_PRINT_CWS(p,c,l) {}
#define DEBUG_PRINT_BITS(p,b,l) {}
#define DEBUG_PRINT_MATRIX(p,m,c,r) {}
#define DEBUG_PRINT_PATTERN_LENGTHS(m,p,r) {}
#define DEBUG_PRINT_PATTERN(m,p,e) {}
#define DEBUG_PRINT_PATTERNS(m,p,e,r) {}

#endif  /* PRNT */

#endif  /* DEBUG_H */
