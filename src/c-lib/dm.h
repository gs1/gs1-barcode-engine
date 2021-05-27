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

#ifndef DM_H
#define DM_H

#define DM_QZ		1
#define MAX_DM_COLS	(144 + 2*DM_QZ)
#define MAX_DM_ROWS	(144 + 2*DM_QZ)
#define MAX_DM_ELMNTS	MAX_DM_COLS		// to accept checkerboard pattern
#define MAX_DM_BYTES	(((MAX_DM_COLS-1)/8+1) * MAX_DM_ROWS)

#define MAX_DM_CWS	2178
#define MAX_DM_DAT_CWS	1558

#define MAX_DM_DAT_CWS_PER_BLK 175
#define MAX_DM_ECC_CWS_PER_BLK 68


#include "gs1encoders.h"


void gs1_DM(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_dm_DM_dataLength(void);
void test_dm_DM_encode(void);

#endif


#endif  /* DM_H */
