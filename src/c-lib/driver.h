/**
 * GS1 barcode encoder library
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

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

#define MAX_LINE 6032 // 10 inches wide at 600 dpi
#define DEFAULT_BMP_FILE "out.bmp"
#define DEFAULT_TIF_FILE "out.tif"

#include "enc-private.h"
#include "gs1encoders.h"

struct sPrints;

#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

// Syntactic sugar for return on failure
#define gs1_driverInit(ctx, xdim, ydim) do {	\
	if (!gs1_doDriverInit(ctx, xdim, ydim))	\
		return;				\
} while(0)

#define gs1_driverAddRow(ctx, prints) do {	\
	if (!gs1_doDriverAddRow(ctx, prints))	\
		return;				\
} while(0)

#define gs1_driverFinalise(ctx) do {	\
	if (!gs1_doDriverFinalise(ctx))	\
		return;			\
} while(0)

bool gs1_doDriverInit(gs1_encoder *ctx, long xdim, long ydim);
bool gs1_doDriverAddRow(gs1_encoder *ctx, struct sPrints *prints);
bool gs1_doDriverFinalise(gs1_encoder *ctx);

#endif /* UTIL_H */
