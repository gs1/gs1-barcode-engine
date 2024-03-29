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

#ifndef GS1_ENCODERS_TEST_H
#define GS1_ENCODERS_TEST_H

#include <stdint.h>

void test_print_codewords(const uint8_t *cws, int numcws);
void test_print_bits(const uint8_t *bytes, int numbits);
void test_print_strings(gs1_encoder *ctx);
bool test_encode(gs1_encoder *ctx, bool should_succeed, int sym, const char* dataStr, const char** expect);


#endif  /* GS1_ENCODERS_TEST_H */
