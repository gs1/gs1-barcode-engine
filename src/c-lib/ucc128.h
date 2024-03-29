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

#ifndef UCC128_H
#define UCC128_H


#define UCC128_MAX_LINHT	500	// Maximum linear height in X
#define UCC128_SYMMAX		53	// UCC/EAN-128 48 symbol chars + strt,FNC1,link,chk & stop max
#define UCC128_MAX_PAT		10574	// 928*8 + 90*(4*8 + 3) for max codewords and 90 rows
#define UCC128_L_PAD		(10-9)	// CCC starts -9X from 1st start bar


#include "enc-private.h"
#include "gs1encoders.h"


void gs1_U128A(gs1_encoder *ctx);
void gs1_U128C(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_ucc_UCC128A_encode(void);
void test_ucc_UCC128C_encode(void);

#endif


#endif
