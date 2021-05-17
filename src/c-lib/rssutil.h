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

#ifndef RSSUTIL_H
#define RSSUTIL_H


#define MAX_SEP_ELMNTS (11*21+4) // for 22 segment RSS Exp
#define MAX_K 14


#include "enc-private.h"
#include "gs1encoders.h"


struct sPrints;

int *gs1_getRSSwidths(gs1_encoder *ctx, int val, int n, int elements, int maxWidth, int noNarrow);
struct sPrints *gs1_cnvSeparator(gs1_encoder *ctx, struct sPrints *prints);

#endif /* RSSUTIL_H */
