/**
 * GS1 Barcode Engine
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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gs1encoders.h"
#include "enc-private.h"


#ifndef SYMBOLOGY
#error
#endif


static gs1_encoder *ctx = NULL;


int LLVMFuzzerInitialize(int *argc, char ***argv) {

	(void)argc;
	(void)argv;

	ctx = gs1_encoder_init(NULL);
	gs1_encoder_setFormat(ctx, gs1_encoder_dRAW);
	gs1_encoder_setSym(ctx, SYMBOLOGY);

	return 0;

}


int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len) {

	char string[MAX_DATA+1];

	if (len > MAX_DATA)
		return 0;

	memcpy(string, buf, len);
	string[len] = '\0';

	gs1_encoder_setDataStr(ctx, string);
	gs1_encoder_encode(ctx);

	return 0;

}
