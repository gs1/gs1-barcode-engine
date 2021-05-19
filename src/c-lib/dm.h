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

#ifndef DM_H
#define DM_H


#define MAX_DM_COLS	146     // include qz
#define MAX_DM_ROWS	146     // include qz
#define MAX_DM_ELMNTS	146     // accept checkerboard pattern
#define MAX_DM_BYTES	2774


#include "gs1encoders.h"


void gs1_DM(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_dm_DM_encode(void);
void test_dm_DM_encode(void);

#endif


#endif  /* DM_H */
