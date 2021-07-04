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

#ifndef EAN_H
#define EAN_H

#include <stdbool.h>

#include "gs1encoders.h"

bool gs1_normaliseEAN13(gs1_encoder *ctx, const char *dataStr, char *primaryStr);
bool gs1_normaliseEAN8(gs1_encoder *ctx, const char *dataStr, char *primaryStr);
bool gs1_normaliseUPCE(gs1_encoder *ctx, const char *dataStr, char *primaryStr);
void gs1_EAN13(gs1_encoder *ctx);
void gs1_EAN8(gs1_encoder *ctx);
void gs1_UPCE(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_ean_EAN13_encode_ean13(void);
void test_ean_EAN13_encode_upca(void);
void test_ean_EAN8_encode(void);
void test_ean_UPCA_encode(void);
void test_ean_UPCE_encode(void);
void test_ean_zeroCompress(void);

#endif


#endif
