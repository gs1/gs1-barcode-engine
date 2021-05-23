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

#define QR_QZ		4
#define MAX_QR_SIZE	(177 + 2*QR_QZ)
#define MAX_QR_ELMNTS	MAX_QR_SIZE		// to accept a checkerboard test pattern
#define MAX_QR_BYTES	(((MAX_QR_SIZE-1)/8+1) * MAX_QR_SIZE)

#define MAX_QR_DATA_BITS	23648
#define MAX_QR_DATA_BYTES	((MAX_QR_DATA_BITS-1)/8+1)
#define MAX_QR_DAT_CWS		2956	// Maximum data codewords (Version 40-L)
#define MAX_QR_CWS		3706	// Maximum overall codewords (Version 40)
#define MAX_QR_DAT_CWS_PER_BLK	128
#define MAX_QR_ECC_CWS_PER_BLK	128


#include "gs1encoders.h"


void gs1_QR(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_qr_QR_fixtures(void);
void test_qr_QR_versions(void);
void test_qr_QR_encode(void);

#endif


#endif  /* QR_H */
