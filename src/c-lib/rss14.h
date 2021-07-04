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

#ifndef RSS14_H
#define RSS14_H

#include <stdbool.h>


#define RSS14_ELMNTS	(46-4)	// not including guard bars
#define RSS14_SYM_W	96	// symbol width in modules including guard bars
#define RSS14_SYM_H	33	// RSS-14 height
#define RSS14_TRNC_H	13	// RSS-14 truncated height
#define RSS14_ROWS1_H	5	// RSS-14S row heights
#define RSS14_ROWS2_H	7
#define RSS14_L_PADR	5	// RSS-14 left offset
#define RSS14_R_PADR	7	// RSS-14s right offset


#include "enc-private.h"
#include "gs1encoders.h"

bool gs1_normaliseRSS14(gs1_encoder *ctx, const char *dataStr, char *primaryStr);
void gs1_RSS14(gs1_encoder *ctx);
void gs1_RSS14S(gs1_encoder *ctx);
void gs1_RSS14SO(gs1_encoder *ctx);


#ifdef UNIT_TESTS

void test_rss14_RSS14_encode(void);
void test_rss14_RSS14T_encode(void);
void test_rss14_RSS14S_encode(void);
void test_rss14_RSS14SO_encode(void);

#endif


#endif
