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

#ifndef ENC_H
#define ENC_H

#include <stdbool.h>

#define GS1_ENCODERS_MAX_FNAME 120
#define GS1_ENCODERS_MAX_DATA (75+2361)

enum {
	sNONE = 0,	// none defined
	sRSS14,		// RSS-14
	sRSS14T,	// RSS-14 Truncated
	sRSS14S,	// RSS-14 Stacked
	sRSS14SO,	// RSS-14 Stacked Omnidirectional
	sRSSLIM,	// RSS Limited
	sRSSEXP,	// RSS Expanded
	sUPCA,		// UPC-A
	sUPCE,		// UPC-E
	sEAN13,		// EAN-13
	sEAN8,		// EAN-8
	sUCC128_CCA,	// UCC/EAN-128 with CC-A or CC-B
	sUCC128_CCC,	// UCC/EAN-128 with CC-C
	sNUMSYMS,	// Number of symbologies
};


/** @brief A gs1_encoder context.
 */
typedef struct gs1_encoder gs1_encoder;

/** @brief Initialise a gs1_encoder instance.
 *  @return gs1_encoder context on success, else NULL.
 */
gs1_encoder* gs1_encoder_init(void);


/** @brief Release a gs1_encoder instance.
 *  @param ctx Instance to free.
 */
void gs1_encoder_free(gs1_encoder *ctx);


/** @brief Encode the barcode symbol.
 *  @param ctx gs1_encoder context.
 */
bool gs1_encoder_encode(gs1_encoder *ctx);


int gs1_encoder_getSym(gs1_encoder *ctx);
void gs1_encoder_setSym(gs1_encoder *ctx, int sym);

int gs1_encoder_getInputFlag(gs1_encoder *ctx);
void gs1_encoder_setInputFlag(gs1_encoder *ctx, int inputFlag);

int gs1_encoder_getPixMult(gs1_encoder *ctx);
void gs1_encoder_setPixMult(gs1_encoder *ctx, int pixMult);

int gs1_encoder_getXundercut(gs1_encoder *ctx);
void gs1_encoder_setXundercut(gs1_encoder *ctx, int Xundercut);

int gs1_encoder_getYundercut(gs1_encoder *ctx);
void gs1_encoder_setYundercut(gs1_encoder *ctx, int Yundercut);

int gs1_encoder_getSepHt(gs1_encoder *ctx);
void gs1_encoder_setSepHt(gs1_encoder *ctx, int sepHt);

int gs1_encoder_getSegWidth(gs1_encoder *ctx);
void gs1_encoder_setSegWidth(gs1_encoder *ctx, int segWidth);

int gs1_encoder_getBmp(gs1_encoder *ctx);
void gs1_encoder_setBmp(gs1_encoder *ctx, int bmp);

int gs1_encoder_getLinHeight(gs1_encoder *ctx);
void gs1_encoder_setLinHeight(gs1_encoder *ctx, int linHeight);

char* gs1_encoder_getOutFile(gs1_encoder *ctx);
void gs1_encoder_setOutFile(gs1_encoder *ctx, char* outFile);

//char* gs1_encoder_getDataStr(gs1_encoder *ctx);
void gs1_encoder_setDataStr(gs1_encoder *ctx, char* dataStr);

//char* gs1_encoder_getDataFile(gs1_encoder *ctx);
void gs1_encoder_setDataFile(gs1_encoder *ctx, char* dataFile);

char* gs1_encoder_getErrMsg(gs1_encoder *ctx);


#endif /* ENC_H */
