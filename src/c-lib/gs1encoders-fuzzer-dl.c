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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gs1encoders.h"
#include "enc-private.h"

static gs1_encoder *ctx = NULL;


int LLVMFuzzerInitialize(int *argc, char ***argv) {

	(void)argc;
	(void)argv;

	ctx = gs1_encoder_init(NULL);

	return 0;

}


int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len) {

	char in[MAX_DATA+1];

	if (len > MAX_DATA)
		return 0;

	memcpy(in, buf, len);
	in[len] = '\0';

	if (!gs1_encoder_setDLuriStr(ctx, in)) {
//		printf("%s\n", gs1_encoder_getErrMsg(ctx));
		return 0;
	}

//	printf("%s => %s\n", gs1_encoder_getDataStr(ctx), gs1_encoder_getAIdataStr(ctx));

/*
	// Maybe something like this eventually - but the conversion isn't
	// bijective
	out = gs1_encoder_getDLuri(ctx);
	if (strcmp(in, out) != 0) {
		printf("\nIN:  %s\nOUT: %s\n", in, out);
		abort();
	}
*/

	return 0;

}
