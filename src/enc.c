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

#include <stdbool.h>
#include <stdint.h>

#include "enc.h"


bool encode(struct sParams *params) {

	FILE *iFile, *oFile;

	errMsg = "";
	errFlag = false;

	if (!params) {
		errFlag = true;
		goto out;
	}

	if (params->inputFlag == 1) {
		size_t i;
		if ((iFile = fopen(params->dataFile, "r")) == NULL) {
			sprintf(errMsg, "UNABLE TO OPEN %s FILE", params->dataFile);
			errFlag = true;
			goto out;
		}
		i = fread(params->dataStr, sizeof(char), MAX_DATA, iFile);
		while (i > 0 && params->dataStr[i-1] < 32) i--; // strip trailing CRLF etc.
		params->dataStr[i] = '\0';
		fclose(iFile);
	}

	if ((oFile = fopen(params->outFile, "wb")) == NULL) {
		sprintf(errMsg, "UNABLE TO OPEN %s FILE", params->outFile);
		errFlag = true;
		goto out;
	}
	params->outfp = oFile;

	switch (params->sym) {

		case sRSS14:
		case sRSS14T:
			RSS14(params);
			break;

		case sRSS14S:
			RSS14S(params);
			break;

		case sRSS14SO:
			RSS14SO(params);
			break;

		case sRSSLIM:
			RSSLim(params);
			break;

		case sRSSEXP:
			RSSExp(params);
			break;

		case sUPCA:
		case sEAN13:
			EAN13(params);
			break;

		case sUPCE:
			UPCE(params);
			break;

		case sEAN8:
			EAN8(params);
			break;

		case sUCC128_CCA:
			U128A(params);
			break;

		case sUCC128_CCC:
			U128C(params);
			break;

		default:
			sprintf(errMsg, "Unknown symbology type %d", params->sym);
			errFlag = true;
			break;

	}

	fclose(oFile);

out:

	params->errMsg = errFlag ? errMsg : "";
	return !errFlag;

}
