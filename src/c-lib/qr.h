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

#ifndef QR_H
#define QR_H


#define MAX_QR_COLS	185	// include qz
#define MAX_QR_ROWS	185	// include qz
#define MAX_QR_ELMNTS	185	// accept checkerboard pattern
#define MAX_QR_BYTES	((MAX_QR_COLS-1)/8+1) * MAX_QR_ROWS


#include "gs1encoders.h"


void gs1_QR(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_qr_QR_encode(void);
void test_qr_QR_encode(void);

#endif


#endif  /* QR_H */