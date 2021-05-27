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
#include <stddef.h>

#define GS1_ENCODERS_MAX_FNAME 120
#define GS1_ENCODERS_MAX_DATA (75+2361)
#define GS1_ENCODERS_MAX_PIXMULT 12
#define GS1_ENCODERS_MAX_KEYDATA 2048
#define GS1_ENCODERS_MAX_LINHT 500     // max UCC/EAN-128 height in X

#ifdef _WIN32
#  define GS1_ENCODERS_API __declspec(dllexport)
#else
#  define GS1_ENCODERS_API
#endif


#ifdef __cplusplus
extern "C" {
#endif


enum {
	gs1_encoder_sNONE = -1,		// none defined
	gs1_encoder_sRSS14,		// RSS-14
	gs1_encoder_sRSS14T,		// RSS-14 Truncated
	gs1_encoder_sRSS14S,		// RSS-14 Stacked
	gs1_encoder_sRSS14SO,		// RSS-14 Stacked Omnidirectional
	gs1_encoder_sRSSLIM,		// RSS Limited
	gs1_encoder_sRSSEXP,		// RSS Expanded
	gs1_encoder_sUPCA,		// UPC-A
	gs1_encoder_sUPCE,		// UPC-E
	gs1_encoder_sEAN13,		// EAN-13
	gs1_encoder_sEAN8,		// EAN-8
	gs1_encoder_sUCC128_CCA,	// UCC/EAN-128 with CC-A or CC-B
	gs1_encoder_sUCC128_CCC,	// UCC/EAN-128 with CC-C
	gs1_encoder_sQR,		// QR Code
	gs1_encoder_sDM,		// Data Matrix
	gs1_encoder_sNUMSYMS,		// Number of symbologies
};


enum {
	gs1_encoder_dBMP = 0,		// BMP format
	gs1_encoder_dTIF = 1,		// TIFF format
	gs1_encoder_dRAW = 2,		// TIFF without header
};


enum {
	gs1_encoder_qrEClevelL = 1,
	gs1_encoder_qrEClevelM,
	gs1_encoder_qrEClevelQ,
	gs1_encoder_qrEClevelH,
};


/** @brief A gs1_encoder context.
 */
typedef struct gs1_encoder gs1_encoder;

GS1_ENCODERS_API size_t gs1_encoder_instanceSize(void);

/** @brief Initialise a gs1_encoder instance.
 *  @return gs1_encoder context on success, else NULL.
 */
GS1_ENCODERS_API gs1_encoder* gs1_encoder_init(void *mem);

GS1_ENCODERS_API char* gs1_encoder_getVersion(gs1_encoder *ctx);

GS1_ENCODERS_API char* gs1_encoder_getErrMsg(gs1_encoder *ctx);

GS1_ENCODERS_API int gs1_encoder_getSym(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setSym(gs1_encoder *ctx, int sym);

GS1_ENCODERS_API int gs1_encoder_getPixMult(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setPixMult(gs1_encoder *ctx, int pixMult);

GS1_ENCODERS_API int gs1_encoder_getXundercut(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setXundercut(gs1_encoder *ctx, int Xundercut);

GS1_ENCODERS_API int gs1_encoder_getYundercut(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setYundercut(gs1_encoder *ctx, int Yundercut);

GS1_ENCODERS_API int gs1_encoder_getSepHt(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setSepHt(gs1_encoder *ctx, int sepHt);

GS1_ENCODERS_API int gs1_encoder_getSegWidth(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setSegWidth(gs1_encoder *ctx, int segWidth);

GS1_ENCODERS_API int gs1_encoder_getLinHeight(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setLinHeight(gs1_encoder *ctx, int linHeight);

GS1_ENCODERS_API int gs1_encoder_getDmRows(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setDmRows(gs1_encoder *ctx, int rows);

GS1_ENCODERS_API int gs1_encoder_getDmColumns(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setDmColumns(gs1_encoder *ctx, int columns);

GS1_ENCODERS_API int gs1_encoder_getQrVersion(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setQrVersion(gs1_encoder *ctx, int version);

GS1_ENCODERS_API int gs1_encoder_getQrEClevel(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setQrEClevel(gs1_encoder *ctx, int ecLevel);

GS1_ENCODERS_API bool gs1_encoder_getFileInputFlag(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setFileInputFlag(gs1_encoder *ctx, bool fileInputFlag);

GS1_ENCODERS_API char* gs1_encoder_getDataStr(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setDataStr(gs1_encoder *ctx, char* dataStr);

GS1_ENCODERS_API char* gs1_encoder_getDataFile(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setDataFile(gs1_encoder *ctx, char* dataFile);

GS1_ENCODERS_API int gs1_encoder_getFormat(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setFormat(gs1_encoder *ctx, int format);

GS1_ENCODERS_API char* gs1_encoder_getOutFile(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setOutFile(gs1_encoder *ctx, char* outFile);

/** @brief Encode the barcode symbol.
 *  @param ctx gs1_encoder context.
 */
GS1_ENCODERS_API bool gs1_encoder_encode(gs1_encoder *ctx);

GS1_ENCODERS_API size_t gs1_encoder_getBuffer(gs1_encoder *ctx, void** out);

GS1_ENCODERS_API int gs1_encoder_getBufferWidth(gs1_encoder *ctx);

GS1_ENCODERS_API int gs1_encoder_getBufferHeight(gs1_encoder *ctx);

GS1_ENCODERS_API size_t gs1_encoder_getBufferStrings(gs1_encoder *ctx, char*** out);

/** @brief Release a gs1_encoder instance.
 *  @param ctx Instance to free.
 */
GS1_ENCODERS_API void gs1_encoder_free(gs1_encoder *ctx);



#ifdef __cplusplus
}
#endif


#endif /* ENC_H */
