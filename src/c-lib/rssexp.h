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

#ifndef RSSEXP_H
#define RSSEXP_H


#define RSSEXP_ELMNTS		21	// 2-segments, no guard patterns
#define RSSEXP_SYM_W		49	// 2-segment symbol width in modules, no guard patterns
#define RSSEXP_SYM_H		34	// height
#define RSSEXP_MAX_DBL_SEGS	12	// max double segments
#define RSSEXP_L_PAD		1	// CC left offset


#include "gs1encoders.h"


void gs1_RSSExp(gs1_encoder *ctx);

#endif
