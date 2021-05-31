/**
 * GS1 Barcode Encoder Library
 *
 * @file gs1encoders.h
 * @author GS1 AISBL
 *
 * \copyright Copyright (c) 2000-2021 GS1 AISBL.
 *
 * @licenseblock{License}
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
 * @endlicenseblock
 *
 */

#ifndef GS1_ENCODERS_H
#define GS1_ENCODERS_H

/// \cond
#include <stdbool.h>
#include <stddef.h>
/// \endcond


// Decorator for public API functions that we export from a DLL
#ifdef _WIN32
#  define GS1_ENCODERS_API __declspec(dllexport)
#else
#  define GS1_ENCODERS_API
#endif


#ifdef __cplusplus
extern "C" {
#endif


/// enum to select a symbology.
enum symbologies {
	gs1_encoder_sNONE = -1,		///< None defined
	gs1_encoder_sRSS14,		///< GS1 DataBar Omnidirectional
	gs1_encoder_sRSS14T,		///< GS1 DataBar Truncated
	gs1_encoder_sRSS14S,		///< GS1 DataBar Stacked
	gs1_encoder_sRSS14SO,		///< GS1 DataBar Stacked Omnidirectional
	gs1_encoder_sRSSLIM,		///< GS1 DataBar Limited
	gs1_encoder_sRSSEXP,		///< GS1 DataBar Expanded (Stacked)
	gs1_encoder_sUPCA,		///< UPC-A
	gs1_encoder_sUPCE,		///< UPC-E
	gs1_encoder_sEAN13,		///< EAN-13
	gs1_encoder_sEAN8,		///< EAN-8
	gs1_encoder_sUCC128_CCA,	///< GS1-128 with CC-A or CC-B
	gs1_encoder_sUCC128_CCC,	///< GS1-128 with CC-C
	gs1_encoder_sQR,		///< (GS1) QR Code
	gs1_encoder_sDM,		///< (GS1) Data Matrix
	gs1_encoder_sNUMSYMS,		///< Value is the number of symbologies
};


/// enum to select the output format.
enum formats {
	gs1_encoder_dBMP = 0,		///< BMP format
	gs1_encoder_dTIF = 1,		///< TIFF format
	gs1_encoder_dRAW = 2,		///< TIFF, without header (1-bit per pixel matrix with byte-aligned rows)
};


/// enum to select QR Code error correction level.
enum qrEClevel {
	gs1_encoder_qrEClevelL = 1,	///< Low error correction (7% damage recovery)
	gs1_encoder_qrEClevelM,		///< Medium error correction (15% damage recovery)
	gs1_encoder_qrEClevelQ,		///< Quartile error correction (25% damage recovery)
	gs1_encoder_qrEClevelH,		///< High error correction (30% damage recovery)
};


/**
 * @brief A gs1_encoder context.
 *
 * This is an opaque struct that represents an encoder context.
 *
 * A context is an "instance" of the library and maintains all state required
 * for that instance. Any number of instances of the library can be created,
 * each operating seperately and equivalently to all others.
 *
 * This library does not introduce and global variables. In general, all state
 * is maintained in instances of the gs1_encoder struct and this state should
 * only be modified using the public API functions provided by this library
 * (decorated with GS1_ENCODERS_API).
 *
 * A context is created by calling gs1_encoder_init() and destroyed by calling
 * gs1_encoder_free().
 *
 * \note
 * This struct is deliberately opaque and it's layout should be assumed to vary
 * between releases of the library and between builds create with different
 * options.
 *
 * \note
 * The library is thread-safe provided that each thread operates on its own
 * instance of the library.
 *
 */
typedef struct gs1_encoder gs1_encoder;


/**
 * @brief Get the version string of the library.
 *
 * This is typically the build date.
 *
 * @param[in] ctx	instance of ::gs1_encoder
 * @return pointer to a string containing the version of the library
 */
GS1_ENCODERS_API char* gs1_encoder_getVersion(gs1_encoder *ctx);


/**
 * @brief Find the memory storage requirements for an instance of ::gs1_encoder.
 *
 * For embedded systems it may be desirable to provide a pre-allocated buffer
 * to a new context for storage purposes, rather than have the instance
 * malloc() it's own storage.
 *
 * This function provides the minimum size required for such a buffer.
 *
 * Example of allocating our own heap storage:
 *
 * \code{.c}
 * gs1_encoder *ctx;
 * size_t mem = gs1_encoder_instanceSize();
 * void *heap = malloc(mem);
 * assert(heap);
 * ctx = gs1_encoder_init(heap);
 * \endcode
 *
 * Example of using static storage:
 *
 * \code{.c}
 * static uint8_t prealloc[65536];
 * ...
 * void myFunc(void) {
 * 	gs1_encoder *ctx;
 * 	size_t mem = gs1_encoder_instanceSize();
 * 	assert(sizeof(prealloc) <= mem);
 * 	ctx = gs1_encoder_init((void *)prealloc);
 * 	...
 * }
 * \endcode
 *
 * @return memory required to hold a context instance
 */
GS1_ENCODERS_API size_t gs1_encoder_instanceSize(void);


/**
 * @brief Get the maximum filename length that can be used for input and output.
 *
 * @return maximum size of filename paths
 */
GS1_ENCODERS_API int gs1_encoder_getMaxFilenameLength(void);


/**
 * @brief Get the maximum size of the input data buffer for barcode message content.
 *
 * This is an implementation limit that may be lowered for systems with limited
 * memory.
 *
 * \note
 * Each barcode symbology has its own data capacity that may be somewhat less
 * than this maximum size.
 *
 * @return maximum number bytes that can be supplied for encoding
 */
GS1_ENCODERS_API int gs1_encoder_getMaxInputBuffer(void);


/**
 * @brief Get the maximum X dimension (in pixels)
 *
 * This is an implementation limit that may be lowered for systems with limited
 * memory.
 *
 * @return maximum number pixel per module ("X-dimension")
 */
GS1_ENCODERS_API int gs1_encoder_getMaxPixMult(void);


/**
 * @brief Get the maximum height of GS1-128 linear symbols in modules
 *
 * This is an implementation limit that may be lowered for systems with limited
 * memory.
 *
 * @return maximum GS1-128 height in modules
 */
GS1_ENCODERS_API int gs1_encoder_getMaxUcc128LinHeight(void);


/**
 * @brief Initialise a new ::gs1_encoder context.
 *
 * If storage is provided then this will be used, however it is more typical to
 * pass NULL in which case the library will malloc its own storage.
 *
 * Any storage buffer provided must be sufficient for the instance.
 *
 * @see gs1_encoder_instanceSize()
 *
 * @param [in,out] mem buffer to use for storage, or NULL for automatic allocation
 * @return ::gs1_encoder context on success, else NULL.
 */
GS1_ENCODERS_API gs1_encoder* gs1_encoder_init(void *mem);


/**
 * @brief Read an error message generated by the library.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return pointer to error message string
 */
GS1_ENCODERS_API char* gs1_encoder_getErrMsg(gs1_encoder *ctx);


/**
 * @brief Get the current symbology type.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return symbology type, one of ::symbologies
 */
GS1_ENCODERS_API int gs1_encoder_getSym(gs1_encoder *ctx);


/**
 * @brief Set the symbology type.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] sym current symbology type, one of ::symbologies other than ::gs1_encoder_sNONE or ::gs1_encoder_sNUMSYMS.
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setSym(gs1_encoder *ctx, int sym);


/**
 * @brief Get the current pixels per module ("X-dimension")
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return pixels per module
 */
GS1_ENCODERS_API int gs1_encoder_getPixMult(gs1_encoder *ctx);


/**
 * @brief Set the pixels per module ("X-dimension").
 *
 * Valid options range from 1 up to the limit returned by gs1_encoder_getMaxPixMult().
 *
 * \note
 * The X and Y undercut will be reset if the new X dimension is insufficient.
 * The separator height will be updated to match the new X dimension as necessary.
 *
 * @see gs1_encoder_getMaxPixMult()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] pixMult pixels per module
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setPixMult(gs1_encoder *ctx, int pixMult);


/**
 * @brief Get the current X undercut.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current X undercut in pixels
 */
GS1_ENCODERS_API int gs1_encoder_getXundercut(gs1_encoder *ctx);


/**
 * @brief Set the X undercut.
 *
 * Compensate for horizontal print growth by shaving this number of pixels from
 * both sides of each module.
 *
 * \note
 * Must be less than half the X-dimension.
 *
 * @see gs1_encoder_getPixMult()
 *
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] Xundercut in pixels
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setXundercut(gs1_encoder *ctx, int Xundercut);


/**
 * @brief Get the current Y undercut.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current Y undercut in pixels
 */
GS1_ENCODERS_API int gs1_encoder_getYundercut(gs1_encoder *ctx);


/**
 * @brief Set the Y undercut.
 *
 * Compensate for vertical print growth by shaving this number of pixels from
 * the top and bottom of each module.
 *
 * \note
 * Must be less than half the X-dimension.
 *
 * @see gs1_encoder_getPixMult()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] Yundercut in pixels
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setYundercut(gs1_encoder *ctx, int Yundercut);


/**
 * @brief Get the current separator height between linear and 2D components.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current serparator height
 */
GS1_ENCODERS_API int gs1_encoder_getSepHt(gs1_encoder *ctx);


/**
 * @brief Set the separator height between linear and 2D components.
 *
 * \note
 * Valid values are 1 to 2 times the X dimension.
 *
 * @see gs1_encoder_getPixMult()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] sepHt in pixels
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setSepHt(gs1_encoder *ctx, int sepHt);


/**
 * @brief Get the current number of segments per row for GS1 DataBar Expanded
 * Stacked symbols.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current segments per row
 */
GS1_ENCODERS_API int gs1_encoder_getRssExpSegWidth(gs1_encoder *ctx);


/**
 * @brief Set the number of segments per row for GS1 DataBar Expanded Stacked
 * symbols.
 *
 * \note
 * Valid values are even numbers from 2 to 22.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] rssExpSegWidth segments per row
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setRssExpSegWidth(gs1_encoder *ctx, int rssExpSegWidth);


/**
 * @brief Get the height of GS1-128 linear symbols in modules.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current linear symbol height in modules
 */
GS1_ENCODERS_API int gs1_encoder_getUcc128LinHeight(gs1_encoder *ctx);


/**
 * @brief Set the height of GS1-128 linear symbols in modules.
 *
 * \note
 * Valid values are 1 to the value returned by gs1_encoder_getMaxUcc128LinHeight().
 *
 * @see gs1_encoder_getMaxUcc128LinHeight()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] ucc128linHeight height in modules
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setUcc128LinHeight(gs1_encoder *ctx, int ucc128linHeight);


/**
 * @brief Get the current fixed number of rows for Data Matrix symbols.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current number of fixed rows, or 0 if automatic
 */
GS1_ENCODERS_API int gs1_encoder_getDmRows(gs1_encoder *ctx);


/**
 * @brief Set a fixed number of rows for Data Matrix symbols.
 *
 * \note
 * Valid values are 8 to 144, or 0 for automatic
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] rows number of fixed rows, or 0 for automatic
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setDmRows(gs1_encoder *ctx, int rows);


/**
 * @brief Get the current fixed number of columns for Data Matrix symbols.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current number of fixed columns, or 0 if automatic
 */
GS1_ENCODERS_API int gs1_encoder_getDmColumns(gs1_encoder *ctx);


/**
 * @brief Set a fixed number of columns for Data Matrix symbols.
 *
 * \note
 * Valid values are 10 to 144, or 0 for automatic
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] columns number of fixed columns, or 0 for automatic
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setDmColumns(gs1_encoder *ctx, int columns);


/**
 * @brief Get the current fixed version number for QR Code symbols.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current version number, or 0 if automatic
 */
GS1_ENCODERS_API int gs1_encoder_getQrVersion(gs1_encoder *ctx);


/**
 * @brief Set a fixed version number for QR Code symbols.
 *
 * \note
 * Valid values are 1 to 40, or 0 for automatic
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] version fixed version number, or 0 for automatic
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setQrVersion(gs1_encoder *ctx, int version);


/**
 * @brief Get the current error correction level for QR Code symbols.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current error correction level, one of ::qrEClevel
 */
GS1_ENCODERS_API int gs1_encoder_getQrEClevel(gs1_encoder *ctx);


/**
 * @brief Set the error correction level for QR Code symbols.
 *
 * This determines what proportion of a symbols data can be reliably
 * reconstructed if it is damaged.
 *
 * Default is ::gs1_encoder_qrEClevelM
 *
 * @see ::qrEClevel
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] ecLevel error correction level, one of ::qrEClevel
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setQrEClevel(gs1_encoder *ctx, int ecLevel);



GS1_ENCODERS_API bool gs1_encoder_getAddCheckDigit(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setAddCheckDigit(gs1_encoder *ctx, bool addCheckDigit);

GS1_ENCODERS_API bool gs1_encoder_getFileInputFlag(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setFileInputFlag(gs1_encoder *ctx, bool fileInputFlag);

GS1_ENCODERS_API char* gs1_encoder_getDataStr(gs1_encoder *ctx);
GS1_ENCODERS_API bool gs1_encoder_setDataStr(gs1_encoder *ctx, char* dataStr);

GS1_ENCODERS_API bool gs1_encoder_setGS1dataStr(gs1_encoder *ctx, char* dataStr);

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


/**
 *  @brief Destroy a gs1_encoder instance.
 *
 *  This will release all library-managed storage associated with the instance.
 *
 *  @param [in,out] ctx ::gs1_encoder context to destroy
 */
GS1_ENCODERS_API void gs1_encoder_free(gs1_encoder *ctx);


#ifdef __cplusplus
}
#endif


#endif /* GS1_ENCODERS_H */
