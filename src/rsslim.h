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

#ifndef RSSLIM_H
#define RSSLIM_H

#include "enc-private.h"

#define RSSLIM_ELMNTS	(46-4)	// not including guard bars
#define RSSLIM_SYM_W	74	// symbol width in modules including any quiet zones
#define RSSLIM_SYM_H	10	// total pixel ht of RSS14L
#define RSSLIM_L_PADB	10	// RSS Limited left pad for ccb

void RSSLim(gs1_encoder *ctx);

#endif
