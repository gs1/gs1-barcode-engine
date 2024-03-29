/**
 * GS1 Barcode Engine
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
 *
 * Overview
 * --------
 *
 * The GS1 Barcode Engine provides routines to generate GS1 barcode symbols,
 * process GS1 Digital Link and GS1 AI syntax data provided in raw or
 * human-friendly formats, and process the scan data received from barcode
 * readers.
 *
 * The symbologies supported by this library are:
 *
 *   * GS1 DataBar family
 *   * GS1-128
 *   * UPC and EAN
 *   * 2D Composite Components are supported for each of the above.
 *   * Data Matrix
 *   * QR Code
 *
 * The encoder implementations are intended for use with GS1 standards and
 * applications and do not contain additional features that might be required
 * for more general use.
 *
 * Within the GS1 Application Identifier system, structured data is represented
 * in different formats depending upon the context.
 *
 * The data formats supported by this library are:
 *
 *   * **Bracketed AI element strings**: Human-friendly rendition of an AI element string.
 *   * **Unbracketed AI element strings**: Rendition of an AI element string that corresponds most directly to encoded barcode data.
 *   * **Scan data**: The result of scanning a symbol with a barcode reader that has AIM symbologies identifiers enabled.
 *   * **Human Readable Interpretation (HRI)**: Representation of the AI data contained within a symbol.
 *   * **Digital Link URIs**
 *
 * This following diagram shows how the library can be used for processing and
 * transformation of GS1 data, indicating which formats are accepted as input,
 * how barcode message data is generated and AI data extracted from the
 * provided input data, and how the given data can be output in various
 * formats.
 *
 * | Data transformation: Inputs, outputs and buffers |
 * | :----------------------------------------------- |
 * | \image html input_output_buffers.svg             |
 *
 * The barcode input data can either be received from a string buffer or read
 * from a file. The barcode symbols be written to a BMP or TIFF file, or
 * accessed by a buffer in either graphical format or as an array of strings.
 *
 * This example use of the library shows how to write the generated barcode
 * image directly to a file:
 *
 * \code
 * #include "gs1encoders.h"
 *
 * gs1_encoder *ctx = gs1_encoder_init(NULL);           // Create a new instance of the library
 * gs1_encoder_setFormat(ctx, gs1_encoder_dBMP);        // Select BMP output
 * gs1_encoder_setOutFile(ctx, "out.bmp");              // Select output to a file
 * gs1_encoder_setSym(ctx, gs1_encoder_sDM);            // Choose the Data Matrix symbology
 * gs1_encoder_setGS1DataStr(ctx,                       // Data provided in GS1 AI syntax
 *        "(01)12345678901231(10)ABC123(11)210630");
 * gs1_encoder_encode(ctx);                             // Generate the image, writing the file
 * gs1_encoder_free(ctx);                               // Release the instance of the library
 * \endcode
 *
 * Another is to capture the graphical output into a buffer or extract the
 * image as a set of strings that can be processed by the application:
 *
 * \code
 * #include "gs1encoders.h"
 *
 * gs1_encoder *ctx = gs1_encoder_init(NULL);           // Create a new instance of the library
 * gs1_encoder_setFormat(ctx, gs1_encoder_dRAW);        // Select RAW output (no graphical header)
 * gs1_encoder_setOutFile(ctx, "");                     // Select output to a buffer
 * gs1_encoder_setSym(ctx, gs1_encoder_sEAN13;          // Choose the EAN-13 symbology
 * gs1_encoder_setGS1DataStr(ctx, "2112345678900");     // Data provided in plain syntax
 * gs1_encoder_encode(ctx);                             // Generate the image, writing the buffer
 *
 * size = gs1_encoder_getBuffer(ctx, buffer);           // Read from the buffer
 *
 * rows = gs1_encoder_getBufferStrings(ctx, strings);   // Alternatively, convert buffer to an array of strings
 *
 * ...
 * gs1_encoder_free(ctx);                               // Release the instance of the library
 * \endcode
 *
 * Most of the setter and action functions of this library return a boolean
 * indicating whether the function was successful and write an error message
 * that can be accessed with gs1_encoder_getErrMsg() in the event of failure.
 * The example code above should be extended to check the return status of each
 * function when used in production code.
 *
 * ### Image scaling models
 *
 * This library generates bitmaps of barcode symbols ready for users of the
 * library to render using whatever display or printing technology is available
 * in their environment.
 *
 * This library does not attempt to interface directly with any specific
 * technology because the number of scenarios is too numerous and the library
 * is not being provided by GS1 AISBL as a turnkey end-user product.
 * Furthermore, the intent is to maintain the portability of this library by
 * avoiding build time and runtime dependancies on other libraries.
 *
 * The library provides two image scaling models:
 *
 * #### 1. Pixel-based scaling system (no real world dimensions)
 *
 * By default, this library creates output that has no regard for real world
 * dimensions. The "PixMult" property is set to 1, which means that each symbol
 * module (narrowest bar or smallest square) is represented by one pixel.
 *
 * The user can set PixMult to any integer scale factor to increase the size of
 * the barcode symbol. Note that a pixel is the "atom" of an digital image and
 * as such there is no realisation of a partial pixel. This has the effect of
 * increasing the output bitmap in quantums which ensures the stability of the
 * relative module sizes in the image.
 *
 * This scaling system is most useful when the barcode is intended to be
 * presented on graphical systems where the precise pitch of the display medium
 * is unknown or cannot be controlled, for example when rendering to multiple
 * different smartphone displays in open applications.
 *
 * @see gs1_encoder_setPixMult()
 *
 *
 * #### 2. Device-dot scaling system (real world dimensions)
 *
 * In systems where the end device resolution is known with certainty the
 * library allows the user to specify this resolution and to provide a target
 * X-dimension size (including optional minimum and maximum permissible limits)
 * in terms of real world dimensions such as mm or inches.
 *
 * This has the effect of automatically selecting a "PixMult" (bitmap pixels
 * per module) that would most closely achieve the specified X-dimension within
 * the provided constraints, when printed at exactly one device dot per bitmap
 * pixel on a device with the specified resolutions. The library will report
 * what X-dimension, if any, has actually been achieved to satisfy the given
 * constraints.
 *
 * It is the user's responsiblity to report the device resolution accurately
 * and to ensure that the integrity of the graphical output from this library
 * is maintained all the way through to the finished result. Specifically, that
 * each source pixel is represented uniquely by a single output device dot in
 * order for the actual X-dimension of the printed image to be correct.
 *
 * @see gs1_encoder_setDeviceResolution()
 * @see gs1_encoder_setXdimension()
 * @see gs1_encoder_getActualXdimension()
 *
 * \note
 * It is the user's responsiblity to ensure that the integrity of the graphical
 * output from this library is maintained all the way through to the finished
 * result.
 * \note
 * Graphics software that uses the bitmap output from this library on
 * pixel/dot-based devices must scale each source pixel exactly to the pixel
 * pitch of the output device. This is typically one bitmap pixel to exactly
 * one device dot is chosen for maximum fidelity. Alternatively, the number of
 * dots comprising each printed module (narrowest bar or smallest square) must
 * be a fixed and constant integer multiple of the number of source pixels for
 * such a module. In this case the user should carefully consider what scaling
 * is in effect when using the real-world X-dimension features of this library.
 * \note
 * Real world printed symbols may experience some degree of print gain due to
 * factors such as ink bleed. This library provides functions to compensate for
 * this by uniformly shaving down the size of bars and modules to the ultimate
 * fidelity of the printer when images are printed as recommended such that one
 * bitmap pixel is rendered by one device dot.
 *
 * @see gs1_encoder_setXundercut()
 * @see gs1_encoder_setYundercut()
 *
 * \note
 * The user should verify the quality of the resulting output and
 * calibrate the output device and/or this library as necessary to optimise the
 * grade of the resulting symbol.
 *
 */


#ifndef GS1_ENCODERS_H
#define GS1_ENCODERS_H

/// \cond
#include <stdbool.h>
#include <stddef.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
/// \endcond


// Decorator for public API functions that we export
#ifdef __EMSCRIPTEN__
#  define GS1_ENCODERS_API EMSCRIPTEN_KEEPALIVE
#elif _WIN32
#  define GS1_ENCODERS_API __declspec(dllexport)
#else
#  define GS1_ENCODERS_API
#endif


#ifdef __cplusplus
extern "C" {
#endif


/// All of the major GS1 barcode formats ("symbologies") are supported by this library.
enum gs1_encoder_symbologies {
	gs1_encoder_sNONE = -1,			///< None defined
	gs1_encoder_sDataBarOmni,		///< GS1 DataBar Omnidirectional
	gs1_encoder_sDataBarTruncated,		///< GS1 DataBar Truncated
	gs1_encoder_sDataBarStacked,		///< GS1 DataBar Stacked
	gs1_encoder_sDataBarStackedOmni,	///< GS1 DataBar Stacked Omnidirectional
	gs1_encoder_sDataBarLimited,		///< GS1 DataBar Limited
	gs1_encoder_sDataBarExpanded,		///< GS1 DataBar Expanded (Stacked)
	gs1_encoder_sUPCA,			///< UPC-A
	gs1_encoder_sUPCE,			///< UPC-E
	gs1_encoder_sEAN13,			///< EAN-13
	gs1_encoder_sEAN8,			///< EAN-8
	gs1_encoder_sGS1_128_CCA,		///< GS1-128 with CC-A or CC-B
	gs1_encoder_sGS1_128_CCC,		///< GS1-128 with CC-C
	gs1_encoder_sQR,			///< (GS1) QR Code
	gs1_encoder_sDM,			///< (GS1) Data Matrix
	gs1_encoder_sNUMSYMS,			///< Value is the number of symbologies
};


/// Barcode images can be written in common BMP and TIFF graphical formats, as
/// well as output as a headerless matrix.
enum gs1_encoder_formats {
	gs1_encoder_dBMP = 0,			///< BMP format
	gs1_encoder_dTIF = 1,			///< TIFF format
	gs1_encoder_dRAW = 2,			///< TIFF, without header (1-bit per pixel matrix with byte-aligned rows)
};


/// The Data Matrix symbols may only be generated with a specific number of
/// rows.
enum gs1_encoder_dmRows {
	gs1_encoder_dmRowsAutomatic = 0,	///< Automatic, based on barcode data
	gs1_encoder_dmRows8 = 8,		///< 8x18, 8x32
	gs1_encoder_dmRows10 = 10,		///< 10x10
	gs1_encoder_dmRows12 = 12,		///< 12x12, 12x26, 12x36
	gs1_encoder_dmRows14 = 14,		///< 14x14
	gs1_encoder_dmRows16 = 16,		///< 16x16, 16x36, 16x48
	gs1_encoder_dmRows18 = 18,		///< 18x18
	gs1_encoder_dmRows20 = 20,		///< 20x20
	gs1_encoder_dmRows22 = 22,		///< 22x22
	gs1_encoder_dmRows24 = 24,		///< 24x24
	gs1_encoder_dmRows26 = 26,		///< 26x26
	gs1_encoder_dmRows32 = 32,		///< 32x32
	gs1_encoder_dmRows36 = 36,		///< 36x36
	gs1_encoder_dmRows40 = 40,		///< 40x40
	gs1_encoder_dmRows44 = 44,		///< 44x44
	gs1_encoder_dmRows48 = 48,		///< 48x48
	gs1_encoder_dmRows52 = 52,		///< 52x52
	gs1_encoder_dmRows64 = 64,		///< 64x64
	gs1_encoder_dmRows72 = 72,		///< 72x72
	gs1_encoder_dmRows80 = 80,		///< 80x80
	gs1_encoder_dmRows88 = 88,		///< 88x88
	gs1_encoder_dmRows96 = 96,		///< 96x96
	gs1_encoder_dmRows104 = 104,		///< 104x104
	gs1_encoder_dmRows120 = 120,		///< 120x120
	gs1_encoder_dmRows132 = 132,		///< 132x132
	gs1_encoder_dmRows144 = 144,		///< 144x144
};


/// The Data Matrix symbols may only be generated with a specific number of
/// columns.
enum gs1_encoder_dmColumns {
	gs1_encoder_dmColumnsAutomatic = 0,	///< Automatic, based on barcode data
	gs1_encoder_dmColumns10 = 10,		///< 10x10
	gs1_encoder_dmColumns12 = 12,		///< 12x12
	gs1_encoder_dmColumns14 = 14,		///< 14x14
	gs1_encoder_dmColumns16 = 16,		///< 16x16, 16x36, 16x48
	gs1_encoder_dmColumns18 = 18,		///< 18x18
	gs1_encoder_dmColumns20 = 20,		///< 20x20
	gs1_encoder_dmColumns22 = 22,		///< 22x22
	gs1_encoder_dmColumns24 = 24,		///< 24x24
	gs1_encoder_dmColumns26 = 26,		///< 26x26, 12x26
	gs1_encoder_dmColumns32 = 32,		///< 32x32
	gs1_encoder_dmColumns36 = 36,		///< 36x36, 12x36, 16x36
	gs1_encoder_dmColumns40 = 40,		///< 40x40
	gs1_encoder_dmColumns44 = 44,		///< 44x44
	gs1_encoder_dmColumns48 = 48,		///< 48x48, 16x48
	gs1_encoder_dmColumns52 = 52,		///< 52x52
	gs1_encoder_dmColumns64 = 64,		///< 64x64
	gs1_encoder_dmColumns72 = 72,		///< 72x72
	gs1_encoder_dmColumns80 = 80,		///< 80x80
	gs1_encoder_dmColumns88 = 88,		///< 88x88
	gs1_encoder_dmColumns96 = 96,		///< 96x96
	gs1_encoder_dmColumns104 = 104,		///< 104x104
	gs1_encoder_dmColumns120 = 120,		///< 120x120
	gs1_encoder_dmColumns132 = 132,		///< 132x132
	gs1_encoder_dmColumns144 = 144,		///< 144x144
};


/// The QR Code symbology supports several error correction levels which allow
/// differing amount of unreadable data to be reconstructed.
enum gs1_encoder_qrEClevel {
	gs1_encoder_qrEClevelL = 1,	///< Low error correction (7% damage recovery)
	gs1_encoder_qrEClevelM,		///< Medium error correction (15% damage recovery)
	gs1_encoder_qrEClevelQ,		///< Quartile error correction (25% damage recovery)
	gs1_encoder_qrEClevelH,		///< High error correction (30% damage recovery)
};


/// The QR Code symbology supports several versions that specify the size of
/// the symbol.
enum gs1_encoder_qrVersion {
	gs1_encoder_qrVersionAutomatic = 0,	///< Automatic, based on barcode data
	gs1_encoder_qrVersion1,			///< Version 1, 21x21
	gs1_encoder_qrVersion2,			///< Version 2, 25x25
	gs1_encoder_qrVersion3,			///< Version 3, 29x29
	gs1_encoder_qrVersion4,			///< Version 4, 33x33
	gs1_encoder_qrVersion5,			///< Version 5, 37x37
	gs1_encoder_qrVersion6,			///< Version 6, 41x41
	gs1_encoder_qrVersion7,			///< Version 7, 45x45
	gs1_encoder_qrVersion8,			///< Version 8, 49x49
	gs1_encoder_qrVersion9,			///< Version 9, 53x53
	gs1_encoder_qrVersion10,		///< Version 10, 57x57
	gs1_encoder_qrVersion11,		///< Version 11, 61x61
	gs1_encoder_qrVersion12,		///< Version 12, 65x65
	gs1_encoder_qrVersion13,		///< Version 13, 69x69
	gs1_encoder_qrVersion14,		///< Version 14, 73x73
	gs1_encoder_qrVersion15,		///< Version 15, 77x77
	gs1_encoder_qrVersion16,		///< Version 16, 81x81
	gs1_encoder_qrVersion17,		///< Version 17, 85x85
	gs1_encoder_qrVersion18,		///< Version 18, 89x89
	gs1_encoder_qrVersion19,		///< Version 19, 93x93
	gs1_encoder_qrVersion20,		///< Version 20, 97x97
	gs1_encoder_qrVersion21,		///< Version 21, 101x101
	gs1_encoder_qrVersion22,		///< Version 22, 105x105
	gs1_encoder_qrVersion23,		///< Version 23, 109x109
	gs1_encoder_qrVersion24,		///< Version 24, 113x113
	gs1_encoder_qrVersion25,		///< Version 25, 117x117
	gs1_encoder_qrVersion26,		///< Version 26, 121x121
	gs1_encoder_qrVersion27,		///< Version 27, 125x125
	gs1_encoder_qrVersion28,		///< Version 28, 129x129
	gs1_encoder_qrVersion29,		///< Version 29, 133x133
	gs1_encoder_qrVersion30,		///< Version 30, 137x137
	gs1_encoder_qrVersion31,		///< Version 31, 141x141
	gs1_encoder_qrVersion32,		///< Version 32, 145x145
	gs1_encoder_qrVersion33,		///< Version 33, 149x149
	gs1_encoder_qrVersion34,		///< Version 34, 153x153
	gs1_encoder_qrVersion35,		///< Version 35, 157x157
	gs1_encoder_qrVersion36,		///< Version 36, 161x161
	gs1_encoder_qrVersion37,		///< Version 37, 165x165
	gs1_encoder_qrVersion38,		///< Version 38, 169x169
	gs1_encoder_qrVersion39,		///< Version 39, 173x173
	gs1_encoder_qrVersion40,		///< Version 49, 177x177
};


/**
 * @brief A gs1_encoder context.
 *
 * This is an opaque struct that represents an instance of the library.
 *
 * This context maintains all state required for an instance. Any number of
 * instances of the library can be created, each operating seperately and
 * equivalently to the others.
 *
 * This library does not introduce any global variables. All runtime state
 * is maintained in instances of the ::gs1_encoder struct and this state should
 * only be modified using the public API functions provided by this library,
 * decorated with GS1_ENCODERS_API.
 *
 * A context is created by calling gs1_encoder_init() and destroyed by calling
 * gs1_encoder_free(), releasing all of the storage allocated by the library
 * for that instance.
 *
 * \note
 * This struct is deliberately opaque and it's layout should be assumed to vary
 * between releases of the library and even between builds created with
 * different options.
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
 * The return data does not need to be free()ed.
 *
 * @return pointer to a string containing the version of the library
 */
GS1_ENCODERS_API char* gs1_encoder_getVersion(void);


/**
 * @brief Find the memory storage requirements for an instance of ::gs1_encoder.
 *
 * For embedded systems it may be desirable to provide a pre-allocated buffer
 * to a new context for storage purposes, rather than to have the instance
 * malloc() it's own storage. This may avoid problems such as heap
 * fragmentation on systems with a poor memory allocator or a restricted
 * working set size.
 *
 * This function returns the minimum size required for such a buffer.
 *
 * Example of a user of the library allocating its own heap storage:
 *
 * \code{.c}
 * gs1_encoder *ctx;
 * size_t mem = gs1_encoder_instanceSize();
 * void *heap = malloc(mem);
 * assert(heap);
 * ctx = gs1_encoder_init(heap);
 * ...
 * gs1_encoder_free(ctx);
 * free(heap);
 * \endcode
 *
 * Example of a user of the library using static storage allocated at compile
 * time:
 *
 * \code{.c}
 * static uint8_t prealloc[65536];  // Ensure that this is big enough
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
 * This is an implementation limit that may be lowered for systems with limited
 * memory by rebuilding the library.
 *
 * @see gs1_encoder_setDataFile()
 *
 * @return maximum length of a filename
 */
GS1_ENCODERS_API int gs1_encoder_getMaxFilenameLength(void);


/**
 * @brief Get the maximum size of the input data buffer for barcode message content.
 *
 * This is an implementation limit that may be lowered for systems with limited
 * memory by rebuilding the library.
 *
 * \note
 * Each barcode symbology has its own data capacity that may be somewhat less
 * than this maximum size.
 *
 * @see gs1_encoder_setDataStr()
 *
 * @return maximum number bytes that can be supplied for encoding
 */
GS1_ENCODERS_API int gs1_encoder_getMaxDataStrLength(void);


/**
 * @brief Get the maximum X-dimension in pixels
 *
 * This is an implementation limit that may be lowered for systems with limited
 * memory by rebuilding the library.
 *
 * @see gs1_encoder_setPixMult()
 * @see gs1_encoder_getPixMult()
 *
 * @return maximum number pixel per module
 */
GS1_ENCODERS_API int gs1_encoder_getMaxPixMult(void);


/**
 * @brief Get the maximum height of GS1-128 linear symbols in modules
 *
 * This is an implementation limit that may be lowered for systems with limited
 * memory by rebuilding the library.
 *
 * @return maximum GS1-128 height in modules
 */
GS1_ENCODERS_API int gs1_encoder_getMaxGS1_128LinearHeight(void);


/**
 * @brief Initialise a new ::gs1_encoder context.
 *
 * If it expected that most users of the library will pass NULL to this
 * function which causes the library will allocate its own storage.
 *
 * If a pointer to a storage buffer is provided then this will be used instead,
 * however this buffer must be sufficient for the needs of the instance as
 * returned by gs1_encoder_instanceSize() and this buffer should not be reused
 * or freed until gs1_encoder_free() is called.
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
 * When any of the setter functions of this library or gs1_encoder_encode()
 * returns false (indicating an error), a human-friendly error message is
 * generated which can be read using this function.
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after any subsequent library function
 * calls.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return pointer to error message string
 */
GS1_ENCODERS_API char* gs1_encoder_getErrMsg(gs1_encoder *ctx);


/**
 * @brief Get the current symbology type.
 *
 * @see gs1_encoder_setSym()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return symbology type, one of ::gs1_encoder_symbologies
 */
GS1_ENCODERS_API int gs1_encoder_getSym(gs1_encoder *ctx);


/**
 * @brief Set the symbology type.
 *
 * This allows the symbology to be specified as any one of the known
 * ::gs1_encoder_symbologies other than ::gs1_encoder_sNONE or ::gs1_encoder_sNUMSYMS.
 *
 * @see ::gs1_encoder_symbologies
 * @see gs1_encoder_getSym()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] sym symbology type
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setSym(gs1_encoder *ctx, int sym);


/**
 * @brief Get the current device dots per module
 *
 * @see gs1_encoder_setPixMult()
 * @see gs1_encoder_getMaxPixMult()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return device dots per module
 */
GS1_ENCODERS_API int gs1_encoder_getPixMult(gs1_encoder *ctx);


/**
 * @brief Set the device dots per module
 *
 * Directly specifies the number of device pixels that shall compose a module
 * within a barcode symbol. A module is defined to be the most narrow element:
 * The shortest width bar for a linear symbology or the smallest square for a
 * 2D symbology.
 *
 * Valid options range from 1 up to the limit returned by gs1_encoder_getMaxPixMult().
 *
 * Calling this function will clear any X-dimension constraints set by
 * gs1_encoder_setXdimension().
 *
 * \note
 * This option is most useful when creating a bitmap for rendering on a digital
 * display where the image size does not directly relate to any real-world
 * dimensions. To specify the intended output size in terms of real-world
 * dimensions use gs1_encoder_setDeviceResolution() and
 * gs1_encoder_setXdimension() instead.
 *
 * \note
 * The X and Y undercut will be reset if the new X dimension is insufficient.
 * The separator height will be updated to match the new X dimension as necessary.
 *
 * @see gs1_encoder_getPixMult()
 * @see gs1_encoder_getMaxPixMult()
 * @see gs1_encoder_setDeviceResolution()
 * @see gs1_encoder_setXdimension()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] pixMult device dots per module
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setPixMult(gs1_encoder *ctx, int pixMult);


/**
 * @brief Get the specified device resolution
 *
 * This is the value specified by the user using gs1_encoder_setDeviceResolution().
 *
 * @see gs1_encoder_setDeviceResolution()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return device resolution, or 0 if undefined
 */
GS1_ENCODERS_API double gs1_encoder_getDeviceResolution(gs1_encoder *ctx);


/**
 * @brief Set the device resolution
 *
 * Specifies the resolution (device dots per unit) of the output device on
 * which the barcode is to be rendered.
 *
 * This function is expected to be used along with gs1_encoder_setXdimension()
 * to automatically set the device dots per module that would produce an image that
 * most-closely matches the specified real-world dimensions when rendered on
 * the device.
 *
 * Calling this function will clear any existing X-dimension constraints, and
 * therefore must be called before gs1_encoder_setXdimension().
 *
 * \note
 * A real-world unit is not specified, however the unit for the value specified
 * here becomes the basis for the values provided to
 * gs1_encoder_setXdimension().  For example, if the device resolution is
 * specified in dots per mm (d/mm) then the X-dimension constraints must also
 * be provided in mm. If dots per inch (DPI) is provided here then the
 * X-dimension must also be specified in inches.
 *
 * @see gs1_encoder_getDeviceResolution()
 * @see gs1_encoder_setXdimension()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] resolution device dots per unit
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setDeviceResolution(gs1_encoder *ctx, double resolution);


/**
 * @brief Set the constraints for the X-dimension width
 *
 * This function will automatically set the number of device dots per module
 * that is required to produce symbols whose X-dimension most-closely meets the
 * provided target X-dimension on a device whose resolution has been previously
 * specified with gs1_encoder_setDeviceResolution(), without exceeding the
 * optional constraints.
 *
 * The following real-world dimensions are accepted:
 *
 *   * Target X-dimension (mandatory):: The ideal X-dimension for the application.
 *   * Minimum X-dimension (optional):: The minimum X-dimension permitted by the application.
 *   * Maximum X-dimension (optional):: The maximum X-dimension permitted by the application.
 *
 * When the minimum and/or maximum X-dimension is specified the chosen device
 * dots per module is guaranteed to not exceed these constraints. With
 * low-resolution devices and tight X-dimension tollerances it may not be
 * possible to achieve an X-dimension that falls within the given constraints
 * since modules must be composed of some whole number of device dots. Put
 * another way, barcode images must be scaled in quantums of device dots to
 * prevent unstable module widths due to pixel-grazing.
 *
 * For example, it is not possible to generate linear symbols intended for
 * general distribution, where the permissible X-dimension is restricted to 0.495 -
 * 0.660mm, on a device whose resolution is 3 dots/mm since 1 device dot is
 * 0.333mm and 2 device dots is 0.667mm. (Note that it is however possible to
 * achieve an acceptable X-dimension on an even lower resolution, 2 dots/mm
 * device since 1 device dot is 0.500mm, which demonstrates that care should
 * exercised when selecting devices for a particular application.)
 *
 * If the required X-dimension constraints can be achieved then the selected
 * device dots per module can be obtained with gs1_encoder_getPixMult() and
 * the actual X-dimension achieved can be obtained with
 * gs1_encoder_getActualXdimension().
 *
 * If it is not possible to achieve the given X-dimension then the function
 * will produce an error and the device dots per module will be set to
 * undefined (0).
 *
 * \note
 * This function is expected to be used along with
 * gs1_encoder_setDeviceResolution() to automatically set the device dots per
 * module that would produce an image that most-closely matches the specified
 * real-world dimensions when rendered on the device. If the output is intended
 * for rendition on digital displays where the symbol size does not relate to a
 * real-world dimension then the device dots per module should be directly specified
 * using gs1_encoder_setPixMult() instead.
 *
 * @see gs1_encoder_setDeviceResolution()
 * @see gs1_encoder_getMinXdimension()
 * @see gs1_encoder_getTargetXdimension()
 * @see gs1_encoder_getMaxXdimension()
 * @see gs1_encoder_getPixMult()
 * @see gs1_encoder_getActualXdimension()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] min minimum permissible X-dimension, or 0 for undefined
 * @param [in] target target X-dimension
 * @param [in] max maximum permissible X-dimension, or 0 for undefined
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setXdimension(gs1_encoder *ctx, double min, double target, double max);


/**
 * @brief Get the current minimum permissible X-dimension
 *
 * This is the value specified by the user using gs1_encoder_setXdimension().
 *
 * \note
 * The units for this parameter are those implicitly considered when specifying
 * the device resolution using gs1_encoder_setDeviceResolution().
 *
 * @see gs1_encoder_setXdimension()
 * @see gs1_encoder_setDeviceResolution()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return minimum X-dimension, or 0 if undefined
 */
GS1_ENCODERS_API double gs1_encoder_getMinXdimension(gs1_encoder *ctx);


/**
 * @brief Get the current maximum permissible X-dimension
 *
 * This is the value specified by the user using gs1_encoder_setXdimension().
 *
 * \note
 * The units for this parameter are those implicitly considered when specifying
 * the device resolution using gs1_encoder_setDeviceResolution().
 *
 * @see gs1_encoder_setXdimension()
 * @see gs1_encoder_setDeviceResolution()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return minimum X-dimension, or 0 if undefined
 */
GS1_ENCODERS_API double gs1_encoder_getMaxXdimension(gs1_encoder *ctx);


/**
 * @brief Get the current target X-dimension specified by the user
 *
 * This is the value specified by the user using gs1_encoder_setXdimension(). It
 * may differ from the actual X-dimension that has been achieved (based on the
 * device resolution) which can be retrieved using
 * gs1_encoder_getActualXdimension().
 *
 * \note
 * The units for this parameter are those implicitly considered when specifying
 * the device resolution using gs1_encoder_setDeviceResolution().
 *
 * @see gs1_encoder_setXdimension()
 * @see gs1_encoder_setDeviceResolution()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return target X-dimension, or 0 if undefined
 */
GS1_ENCODERS_API double gs1_encoder_getTargetXdimension(gs1_encoder *ctx);


/**
 * @brief Get the actual X-dimension that can be achieved on the device
 *
 * This returns the actual X-dimension that can be achieved based on the device
 * resolution specified by gs1_encoder_setDeviceResolution() and the current
 * number of pixels per module.
 *
 * It likely differs from the target X-dimension but is guaranteed not to
 * exceed any minimum and maximum X-dimension specified using
 * gs1_encoder_setXdimension().
 *
 * \note
 * The units for this parameter are those implicitly considered when specifying
 * the device resolution using gs1_encoder_setDeviceResolution().
 *
 * @see gs1_encoder_setXdimension()
 * @see gs1_encoder_setDeviceResolution()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return achieved X-dimension, or 0 if undefined
 */
GS1_ENCODERS_API double gs1_encoder_getActualXdimension(gs1_encoder *ctx);


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
 * @see gs1_encoder_getXundercut()
 * @see gs1_encoder_getPixMult()
 *
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] Xundercut in pixels
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setXundercut(gs1_encoder *ctx, int Xundercut);


/**
 * @brief Get the current Y undercut.
 *
 * @see gs1_encoder_setYundercut()
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
 * @see gs1_encoder_getYundercut()
 * @see gs1_encoder_getPixMult()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] Yundercut in pixels
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setYundercut(gs1_encoder *ctx, int Yundercut);


/**
 * @brief Get the current separator height between linear and 2D components.
 *
 * @see gs1_encoder_setSepHt()
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
 * @see gs1_encoder_getSepHt()
 * @see gs1_encoder_getPixMult()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] sepHt in pixels
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setSepHt(gs1_encoder *ctx, int sepHt);


/**
 * @brief Get the current number of segments per row for GS1 DataBar Expanded
 * Stacked symbols.
 *
 * @see gs1_encoder_setDataBarExpandedSegmentsWidth()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current segments per row
 */
GS1_ENCODERS_API int gs1_encoder_getDataBarExpandedSegmentsWidth(gs1_encoder *ctx);


/**
 * @brief Set the number of segments per row for GS1 DataBar Expanded Stacked
 * symbols.
 *
 * The default number of segments per row (22) matches the maximum number of
 * symbol character for GS1 DataBar Expanded, resulting in a non-stacked
 * symbol.
 *
 * @see gs1_encoder_getDataBarExpandedSegmentsWidth()
 *
 * \note
 * Valid values are even numbers from 2 to 22.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] dataBarExpandedSegmentsWidth segments per row
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setDataBarExpandedSegmentsWidth(gs1_encoder *ctx, int dataBarExpandedSegmentsWidth);


/**
 * @brief Get the height of GS1-128 linear symbols in modules.
 *
 * @see gs1_encoder_setGS1_128LinearHeight
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current linear symbol height in modules
 */
GS1_ENCODERS_API int gs1_encoder_getGS1_128LinearHeight(gs1_encoder *ctx);


/**
 * @brief Set the height of GS1-128 linear symbols in modules.
 *
 * \note
 * Valid values are 1 to the value returned by gs1_encoder_getMaxGS1_128LinearHeight().
 *
 * @see gs1_encoder_getGS1_128LinearHeight()
 * @see gs1_encoder_getMaxGS1_128LinearHeight()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] gs1_128LinearHeight height in modules
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setGS1_128LinearHeight(gs1_encoder *ctx, int gs1_128LinearHeight);


/**
 * @brief Get the current fixed number of rows for Data Matrix symbols.
 *
 * @see gs1_encoder_setDmRows()
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
 * @see gs1_encoder_getDmRows
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] rows number of fixed rows, or 0 for automatic
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setDmRows(gs1_encoder *ctx, int rows);


/**
 * @brief Get the current fixed number of columns for Data Matrix symbols.
 *
 * @see gs1_encoder_setDmColumns()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current number of fixed columns, or 0 if automatic
 */
GS1_ENCODERS_API int gs1_encoder_getDmColumns(gs1_encoder *ctx);


/**
 * @brief Set a fixed number of columns for Data Matrix symbols.
 *
 * @see gs1_encoder_getDmColumns()
 *
 * \note
 * Valid values are 10 to 144, or 0 for automatic
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] columns number of fixed columns, or 0 for automatic
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setDmColumns(gs1_encoder *ctx, int columns);


/**
 * @brief Get the current fixed version number for QR Code symbols.
 *
 * @see gs1_encoder_setQrVersion()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current version number, one of ::gs1_encoder_qrVersion
 */
GS1_ENCODERS_API int gs1_encoder_getQrVersion(gs1_encoder *ctx);


/**
 * @brief Set a fixed version number for QR Code symbols.
 *
 * Default is ::gs1_encoder_qrVersionAutomatic
 *
 * \note
 * Valid values are 1 to 40, or 0 for automatic
 *
 * @see ::gs1_encoder_qrVersion
 * @see gs1_encoder_getQrVersion()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] version fixed version number, one of ::gs1_encoder_qrVersion
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setQrVersion(gs1_encoder *ctx, int version);


/**
 * @brief Get the current error correction level for QR Code symbols.
 *
 * @see gs1_encoder_setQrEClevel()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current error correction level, one of ::gs1_encoder_qrEClevel
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
 * @see ::gs1_encoder_qrEClevel
 * @see gs1_encoder_getQrEClevel()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] ecLevel error correction level, one of ::gs1_encoder_qrEClevel
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setQrEClevel(gs1_encoder *ctx, int ecLevel);


/**
 * @brief Get the current status of the "add check digit" mode.
 *
 * @see gs1_encoder_setAddCheckDigit()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current status of the add check digit mode
 */
GS1_ENCODERS_API bool gs1_encoder_getAddCheckDigit(gs1_encoder *ctx);


/**
 * @brief Enable or disable "add check digit" mode for EAN/UPC and GS1 DataBar
 * symbols.
 *
 *   * If false (default), then the data string must contain a valid check digit.
 *   * If true, then the data string must not contain a check digit as one will
 *     be generated automatically.
 *
 * \note
 * This option is only valid for symbologies that accept fixed-length data,
 * specifically EAN/UPC and GS1 DataBar except Expanded (Stacked).
 *
 * @see gs1_encoder_getAddCheckDigit()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] addCheckDigit enabled if true; disabled if false
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setAddCheckDigit(gs1_encoder *ctx, bool addCheckDigit);


/**
 * @brief Get the current status of the "permit unknown AIs" mode.
 *
 * @see gs1_encoder_setPermitUnknownAIs()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return current status of the permit unknown AIs mode
 */
GS1_ENCODERS_API bool gs1_encoder_getPermitUnknownAIs(gs1_encoder *ctx);


/**
 * @brief Enable or disable "permit unknown AIs" mode for parsing of bracketed
 * AI element strings and Digital Link URIs.
 *
 *   * If false (default), then all AIs represented by the input data must be
 *     known.
 *   * If true, then unknown AIs (those not in this library's static AI table)
 *     will not be accepted.
 *
 * \note
 * The option only applies to parsed input data, specifically bracketed AI data
 * supplied with gs1_encoder_setAIdata() and Digital Link URIs supplied with
 * gs1_encoder_setDataStr(). Unbracketed AI element strings containing unknown
 * AIs cannot be parsed because it is not possible to differentiate the AI from
 * its data value when the length of the AI is uncertain.
 *
 * @see gs1_encoder_getPermitUnknownAIs()
 * @see gs1_encoder_setAIdata()
 * @see gs1_encoder_setDataStr()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] permitUnknownAIs enabled if true; disabled if false
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setPermitUnknownAIs(gs1_encoder *ctx, bool permitUnknownAIs);


/**
 * @brief Indicates whether barcode data input is currently taken from a buffer
 * or a file.
 *
 * @see gs1_encoder_setFileInputFlag()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return true if input is from a file; false if input is from a buffer
 */
GS1_ENCODERS_API bool gs1_encoder_getFileInputFlag(gs1_encoder *ctx);


/**
 * @brief Selects between a file and buffer for barcode data input.
 * symbols.
 *
 *   * If false (default), then the content of the buffer set by
 *     gs1_encoder_setDataStr() is used from barcode data input.
 *   * If true, then the file specified by gs1_encoder_setDataFile() is used
 *     from barcode data input.
 *
 * \note
 * When a file is used for data input, any trailing newline character is
 * stripped.
 *
 * @see gs1_encoder_getFileInputFlag)
 * @see gs1_encoder_setDataFile()
 * @see gs1_encoder_setDataStr()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] fileInputFlag file input if true; buffer input if false
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setFileInputFlag(gs1_encoder *ctx, bool fileInputFlag);


/**
 * @brief Reads the raw barcode data input buffer.
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to library
 * function that modify the input data buffer such as gs1_encoder_setDataStr(),
 * gs1_encoder_setAIdataStr() and gs1_encoder_setScanData().
 *
 * @see gs1_encoder_getDataStr()
 * @see gs1_encoder_setFileInputFlag()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return a pointer to the data input buffer
 */
GS1_ENCODERS_API char* gs1_encoder_getDataStr(gs1_encoder *ctx);


/**
 * @brief Sets the raw data in the buffer that is to be directly encoded within
 * a barcode symbol when buffer input is selected.
 *
 * Each encoder accepts input in a format that is specific to the requirements
 * of the symbology, as described below. Some accept "plain" data
 * input such as a 13-character GTIN in the case of EAN-13. Some require AI
 * syntax data such as the GS1 DataBar family. Others accept either an AI
 * string or a GS1 Digital Link URI, such as QR Code and Data Matrix.
 *
 * A "^" character at the start of the input indicates that the data is in GS1
 * Application Identifier syntax. In this case, all subsequent instances of the
 * "^" character represent the FNC1 non-data characters that are used to
 * separate fields that are not specified as being pre-defined length from
 * subsequent fields.
 *
 * Inputs beginning with "^" will be validated against certain data syntax
 * rules for GS1 AIs. If the input is invalid then this function will return
 * false and an error message will be set that can be read using
 * gs1_encoder_getErrMsg().
 *
 * It is strongly advised that GS1 data input is instead specified using
 * gs1_encoder_setAIdataStr() which takes care of the AI encoding rules
 * automatically, including insertion of FNC1 characters where required. This
 * can be used for all symbologies that accept GS1 AI syntax data.
 *
 * Inputs beginning with "http://" or "https://" will be parsed as a Digital
 * Link URI during which the corresponding AI element string is extracted and
 * validated.
 *
 * The acceptable data input varies between symbologies:
 *
 *   * **EAN-13**:: 13 digits including check digit
 *   * **EAN-8**:: 8 digits including check digit
 *   * **UPC-A**:: 12 digits including check digit
 *   * **UPC-E**:: 12 digits (not zero-suppressed) including check digit
 *   * **GS1 DataBar Omnidirectional**:: 14 digits including check digit
 *   * **GS1 DataBar Truncated**:: 14 digits including check digit
 *   * **GS1 DataBar Stacked**:: 14 digits including check digit
 *   * **GS1 DataBar Stacked Omnidirectional**:: 14 digits including check digit
 *   * **GS1 DataBar Limited**:: 14 digits beginning "0" or "1" including check digit
 *   * **GS1 DataBar Expanded (Stacked)**:: GS1 AI syntax in raw form, with "^" = FNC1
 *   * **GS1-128**:: GS1 AI syntax in raw form, with "^" = FNC1
 *   * **GS1 DataMatrix**:: GS1 AI syntax in raw form, with "^" = FNC1
 *   * **GS1 QR Code**:: GS1 AI syntax in raw form, with "^" = FNC1
 *   * **Data Matrix**:: A Digital Link URI
 *   * **QR Code**:: A Digital Link URI
 *
 * In the interest of data harmony, the EAN/UPC symbologies will additionally
 * accept a GTIN input, such as (01)00000002345673 for EAN-8.
 *
 * EAN/UPC, GS1 DataBar and GS1-128 support a Composite Component. The
 * Composite Component must be specified in AI syntax. It must be separated
 * from the primary linear components with a "|" character and begin with an
 * FNC1 in first position, for example:
 *
 * \code
 * ^0112345678901231|^10ABC123^11210630
 * \endcode
 *
 * The above specifies a linear component representing "(01)12345678901231"
 * together with a composite component representing "(10)ABC123(11)210630".
 *
 * **Again, for GS1 data it is simpler and less error prone to specify the input
 * in human-friendly GS1 AI syntax using gs1_encoder_setAIdataStr().**
 *
 * \note
 * Plain data inputs (including Digital Link URIs) and AI data inputs that are
 * syntactically valid but unsuitable for the intended symbology will be
 * accepted by this function and this fail when gs1_encoder_encode() is called.
 *
 * \note
 * The length of the data must be less that the value returned by
 * gs1_encoder_getMaxDataStrLength().
 *
 * @see gs1_encoder_setAIdataStr()
 * @see gs1_encoder_getMaxDataStrLength()
 * @see gs1_encoder_getDataStr()
 * @see gs1_encoder_setFileInputFlag()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] dataStr containing the raw barcode data
 * @return true on success, otherwise false and an error message is set that can be read using gs1_encoder_getErrMsg()
 */
GS1_ENCODERS_API bool gs1_encoder_setDataStr(gs1_encoder *ctx, const char *dataStr);


/**
 * @brief Sets the data in the buffer that is used when buffer input is
 * selected by parsing input provided in GS1 Application Identifier syntax into
 * a raw data string.
 *
 * The input is provided in human-friendly format **without** FNC1 characters
 * which are inserted automatically, for example:
 *
 * \code
 * (01)12345678901231(10)ABC123(11)210630
 * \endcode
 *
 * This syntax harmonises the format for the input accepted by all symbologies.
 * For example the following input is acceptable for EAN-13, UPC-A, UPC-E, any
 * variant of the GS1 DataBar family, GS1 QR Code and GS1 DataMatrix:
 *
 * \code
 * (01)00031234000054
 * \endcode
 *
 * The input is immediately parsed and validated against certain rules for GS1
 * AIs, after which the resulting encoding for valid inputs is available via
 * gs1_encoder_getDataStr(). If the input is invalid then this function will
 * return false and an error message will be set that can be read using
 * gs1_encoder_getErrMsg().
 *
 * Any "(" characters in AI element values must be escaped as "\(" to avoid
 * conflating them with the start of the next AI.
 *
 * For symbologies that support a composite component (all except
 * ::gs1_encoder_sDM and ::gs1_encoder_sQR), the data for the linear and 2D
 * components can be separated by a "|" character, for example:
 *
 * \code
 * (01)12345678901231|(10)ABC123(11)210630
 * \endcode
 *
 * \note
 * The ultimate length of the encoded data must be less that the value returned by
 * gs1_encoder_getMaxDataStrLength().
 *
 * @see gs1_encoder_setDataStr()
 * @see gs1_encoder_getMaxDataStrLength()
 * @see gs1_encoder_getDataStr()
 * @see gs1_encoder_setFileInputFlag()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] dataStr the barcode input data in GS1 Application Identifier syntax
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setAIdataStr(gs1_encoder *ctx, const char *dataStr);


/**
 * @brief Return the barcode input data buffer in human-friendly AI syntax
 *
 * For example, if the input data buffer were to contain:
 *
 *     ^011231231231233310ABC123|^99XYZ(TM) CORP
 *
 * Then this function would return:
 *
 *     (01)12312312312333(10)ABC123|(99)XYZ\(TM) CORP
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to library
 * functions that modify the input data buffer such as
 * gs1_encoder_setDataStr(), gs1_encoder_setAIdataStr() and
 * gs1_encoder_setScanData().
 *
 * \note
 * The returned pointer should be checked for NULL which indicates non-AI input data.
 *
 * @see gs1_encoder_getDataStr()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return a pointer to a string representing the data input buffer in AI syntax, or a null pointer if the input data does not contain AI data
 */
GS1_ENCODERS_API char* gs1_encoder_getAIdataStr(gs1_encoder *ctx);


/**
 * @brief Process scan data received from a barcode reader with reporting of
 * AIM symbology identifiers enabled to extract the message data and perform
 * syntax checks in the case of GS1 Digital Link and AI data input.
 *
 * This function will process scan data (such as the output of a barcode
 * reader) and process the received data, setting the data input buffer to the
 * message received and setting the selected symbology to something that is
 * able to carry the received data.
 *
 * \note
 * In some instances the symbology determined by this library will not match
 * that of the image that was scanned. The AIM symbology identifier prefix of the
 * scan data does not always uniquely identify the symbology that was scanned.
 * For example GS1-128 Composite symbols share the same symbology identifier as
 * the GS1 DataBar family.
 *
 * A literal "|" character may be included in the scan data to indicates the
 * separation between the first and second messages that would be transmitted
 * by a reader that is configured to return the composite component when
 * reading EAN/UPC symbols.
 *
 * This example use of the library processes a given scan data string, which is
 * assumed to represent AI data in this instance, and then renders the AI data
 * in human-friendly format.
 *
 * \code
 * #include <stdio.h>
 * #include "gs1encoders.h"
 *
 * gs1_encoder *ctx = gs1_encoder_init(NULL);                  // Create a new instance of the library
 * if (!gs1_encoder_setScanData(ctx,                           // Process the scan data, setting dataStr and Sym)
 *        "]C1011231231231233310ABC123{GS}99TESTING"))
 *     exit 1;                                                 // Handle failure if bad AI data is received
 * printf("AI data: %s\n", gs1_encoder_setGS1DataStr(ctx));    // Print the AI scan data in human-friendly format
 * gs1_encoder_free(ctx);                                      // Release the instance of the library
 * \endcode
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to library
 * functions that modify the input data buffer such as
 * gs1_encoder_setDataStr(), gs1_encoder_setAIdataStr() and
 * gs1_encoder_setScanData().
 *
 * @see gs1_encoder_setScanData()
 * @see gs1_encoder_getDataStr()
 * @see gs1_encoder_getAIdataStr()
 * @see gs1_encoder_getSym()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] scanData the scan data input as read by a reader with AIM symbology identifiers enabled
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setScanData(gs1_encoder* ctx, const char *scanData);


/**
 * @brief Returns the string that should be returned by scanners when reading a
 * symbol that is an instance of the selected symbology and contains the input
 * data.
 *
 * Examples:
 *
 * Symbology                  | Input data                                     | Returned scan data
 * -------------------------- | ---------------------------------------------- | -------------------------------------------------
 * ::gs1_encoder_sEAN13       | 2112345678900                                  | ]E02112345678900
 * ::gs1_encoder_sUPCA        | 416000336108                                   | ]E00416000336108
 * ::gs1_encoder_sEAN8        | 02345673                                       | ]E402345673
 * ::gs1_encoder_sEAN8        | 02345673\|^99COMPOSITE^98XYZ                   | ]E402345673\|]e099COMPOSITE{GS}98XYZ
 * ::gs1_encoder_sGS1_128_CCA | ^011231231231233310ABC123^99TESTING            | ]C1011231231231233310ABC123{GS}99TESTING
 * ::gs1_encoder_sGS1_128_CCA | ^0112312312312333\|^98COMPOSITE^97XYZ          | ]e00112312312312333{GS}98COMPOSITE{GS}97XYZ
 * ::gs1_encoder_sQR          | https://example.org/01/12312312312333          | ]Q1https://example.org/01/12312312312333
 * ::gs1_encoder_sQR          | ^01123123123123338200http://example.com        | ]Q301123123123123338200http://example.com
 * ::gs1_encoder_sDM          | https://example.com/gtin/09506000134352/lot/A1 | ]d1https://example.com/gtin/09506000134352/lot/A1
 * ::gs1_encoder_sDM          | ^011231231231233310ABC123^99TESTING            | ]d2011231231231233310ABC123{GS}99TESTING
 *
 * The output will be prefixed with the appropriate AIM symbology identifier.
 *
 * "{GS}" in the scan data output in the above table represents a literal GS
 * character (ASCII 29) that is included in the returned data.
 *
 * The literal "|" character included in the scan data output for EAN/UPC
 * Composite symbols indicates the separation between the first and second
 * messages that would be transmitted by a reader that is configured to return
 * the composite component.
 *
 * This example use of the library shows how to determine what data a scanner
 * should provide when reading a certain symbol:
 *
 * \code
 * #include <stdio.h>
 * #include "gs1encoders.h"
 *
 * gs1_encoder *ctx = gs1_encoder_init(NULL);               // Create a new instance of the library
 * gs1_encoder_setSym(ctx, gs1_encoder_sDataBarExpanded);   // Choose the symbology
 * gs1_encoder_setGS1DataStr(ctx,                           // Set the input data (AI format on this occasion)
 *        "(01)12345678901231(10)ABC123(11)210630");
 * printf("Scan data: %s\n", gs1_encoder_getScanData(ctx)); // Print the scan data that a reader should return
 * gs1_encoder_free(ctx);                                   // Release the instance of the library
 * \endcode
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to library
 * functions that modify the input data buffer such as
 * gs1_encoder_setDataStr(), gs1_encoder_setAIdataStr() and
 * gs1_encoder_setScanData().
 *
 * @see gs1_encoder_setScanData()
 * @see gs1_encoder_setDataStr()
 * @see gs1_encoder_setAIdataStr()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return a pointer to a string representing the scan data for the input data contained within symbols of the selected symbology
 */
GS1_ENCODERS_API char* gs1_encoder_getScanData(gs1_encoder* ctx);


/**
 * @brief Update a given pointer towards an array of strings containing
 * Human-Readable Interpretation ("HRI") text.
 *
 * For example, if the input data buffer were to contain:
 *
 *     ^011231231231233310ABC123|^99XYZ(TM) CORP
 *
 * Then this function would return the following array of null-terminated
 * strings:
 *
 *     "(01) 12312312312333"
 *     "(10) ABC123"
 *     "--"
 *     "(99) XYZ(TM) CORP"
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to functions
 * that modify the input data buffer such as gs1_encoder_setDataStr(),
 * gs1_encoder_setAIdataStr() or gs1_encoder_setScanData().
 *
 * @see gs1_encoder_getDataStr()
 * @see gs1_encoder_setFileInputFlag()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [out] hri Pointer to an array of HRI strings
 * @return the number of HRI strings
 */
GS1_ENCODERS_API int gs1_encoder_getHRI(gs1_encoder* ctx, char ***hri);


/**
 * @brief Get the require HRI buffer size.
 *
 * @see gs1_encoder_copyHRI()
 *
 * \note
 * This function is useful when used in tandem with gs1_encoder_copyHRI() to
 * determine the size of the user-provided buffer to pre-allocate.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return required length of the buffer
 */
GS1_ENCODERS_API size_t gs1_encoder_getHRIsize(gs1_encoder *ctx);


/**
 * @brief Copy the HRI to a user-provided buffer in the form of a "|"-separated string.
 *
 * The buffer into which the output buffer is copied must be preallocated with
 * at least the size returned by gs1_encoder_getHRIsize().
 *
 * @see gs1_encoder_getHRIsize()
 *
 * \note
 * This function exists mainly for environments where it is difficult to access
 * library-allocated buffers via a pointer modified in function parameters,
 * as it the case with Emscripten.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [out] buf a pointer to a buffer into which the formatted HRI text is copied
 * @param [in] max the maximum number of bytes that may be copied into the provided buffer
 */
GS1_ENCODERS_API void gs1_encoder_copyHRI(gs1_encoder *ctx, void *buf, size_t max);


/**
 * @brief Gets the filename for a file containing the barcode data when file
 * input is selected.
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to
 * setDataFile().
 *
 * @see gs1_encoder_setDataFile()
 * @see gs1_encoder_setFileInputFlag()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return a pointer to a string containing the filename
 */
GS1_ENCODERS_API char* gs1_encoder_getDataFile(gs1_encoder *ctx);


/**
 * @brief Sets the filename for a file from which barcode data is read when file
 * input is selected.
 *
 * \note
 * The length of the filename must be less than the value returned by gs1_encoder_getMaxFilenameLength().
 *
 * @see gs1_encoder_getDataFile()
 * @see gs1_encoder_setFileInputFlag()
 * @see gs1_encoder_getMaxFilenameLength()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] dataFile the data input filename
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setDataFile(gs1_encoder *ctx, const char *dataFile);


/**
 * @brief Get the current output format type.
 *
 * @see gs1_encoder_setFormat()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return output format, one of ::gs1_encoder_formats
 */
GS1_ENCODERS_API int gs1_encoder_getFormat(gs1_encoder *ctx);


/**
 * @brief Set the output format for the barcode symbol.
 *
 * This allows the output format to be specified as any one of the known
 * ::gs1_encoder_formats:
 *
 *   * ::gs1_encoder_dBMP: BMP format
 *   * ::gs1_encoder_dTIF: TIFF format
 *   * ::gs1_encoder_dRAW: TIFF format, without the header
 *
 * @see gs1_encoder_getFormat()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] format output format, one of ::gs1_encoder_formats
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setFormat(gs1_encoder *ctx, int format);


/**
 * @brief Get the current output filename.
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent calls to
 * setOutFile().
 *
 * @see gs1_encoder_setOutFile()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return output filename
 */
GS1_ENCODERS_API char* gs1_encoder_getOutFile(gs1_encoder *ctx);


/**
 * @brief Set the output file for the barcode symbol.
 *
 * The output will be written to the specified file in the format specified by
 * gs1_encoder_setFormat().
 *
 * If the filename is provided as the empty string then the output data will be
 * stored in the output buffer which can be read using gs1_encoder_getBuffer()
 * and gs1_encoder_getBufferStrings().
 *
 * \note
 * The length of the filename must be less than the value returned by
 * gs1_encoder_getMaxFilenameLength().
 *
 * @see gs1_encoder_getOutFile()
 * @see gs1_encoder_getBuffer()
 * @see gs1_encoder_getBufferStrings()
 * @see gs1_encoder_getMaxFilenameLength()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [in] outFile the output filename, or "" for buffer output
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_setOutFile(gs1_encoder *ctx, const char *outFile);


/**
 * @brief Generate a barcode symbol representing the given input data
 *
 * This will create a barcode image for the symbology specified by
 * gs1_encoder_setSym() containing the data provided by
 * gs1_encoder_setDataStr() or gs1_encoder_setAIdataStr().
 *
 * If the input is valid for the selected symbology then the image output will
 * be written to the output file specified by gs1_encoder_setOutFile(), or to
 * the output buffer if the output filename is empty, in the format specified
 * by gs1_encoder_setFormat().
 *
 * @see gs1_encoder_setSym()
 * @see gs1_encoder_setDataStr()
 * @see gs1_encoder_setAIdataStr()
 * @see gs1_encoder_setFormat()
 * @see gs1_encoder_setOutFile()
 *
 * @param ctx gs1_encoder context.
 * @return true on success, otherwise false and an error message is set
 */
GS1_ENCODERS_API bool gs1_encoder_encode(gs1_encoder *ctx);


/**
 * @brief Get the output buffer.
 *
 * @see gs1_encoder_setOutFile()
 *
 * \note
 * The return data does not need to be free()ed and the content should be
 * copied if it must persist in user code after subsequent library function
 * calls.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [out] buffer a pointer to the buffer
 * @return length of the buffer
 */
GS1_ENCODERS_API size_t gs1_encoder_getBuffer(gs1_encoder *ctx, void **buffer);


/**
 * @brief Get the required output buffer size.
 *
 * @see gs1_encoder_copyOutputBuffer()
 *
 * \note
 * This function is useful when used in tandem with
 * gs1_encoder_copyOutputBuffer() to determine the size of the user-provided
 * buffer to pre-allocate.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return required length of the buffer
 */
GS1_ENCODERS_API size_t gs1_encoder_getBufferSize(gs1_encoder *ctx);


/**
 * @brief Copy the output buffer to a user-provided buffer.
 *
 * The buffer into which the output buffer is copied must be preallocated with
 * at least the size returned by gs1_encoder_getBufferSize().
 *
 * @see gs1_encoder_getBufferSize()
 *
 * \note
 * This function exists mainly for environments where it is difficult to access
 * library-allocated buffers via a pointer modified in function parameters,
 * as it the case with Emscripten.
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [out] buffer a pointer to a buffer into which the output buffer is copied
 * @param [in] max the maximum number of bytes that may be copied into the provided buffer
 * @return number of bytes copied, or 0 if the provided buffer is insufficient for the entire output
 */
GS1_ENCODERS_API size_t gs1_encoder_copyOutputBuffer(gs1_encoder *ctx, void *buffer, size_t max);


/**
 * @brief Get the number of columns in the output buffer image.
 *
 * @see gs1_encoder_getBuffer()
 * @see gs1_encoder_getBufferStrings()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return the number of columns of the image held in the buffer
 */
GS1_ENCODERS_API int gs1_encoder_getBufferWidth(gs1_encoder *ctx);


/**
 * @brief Get the number of rows in the output buffer image.
 *
 * @see gs1_encoder_getBuffer()
 * @see gs1_encoder_getBufferStrings()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @return the number of rows of the image held in the buffer
 */
GS1_ENCODERS_API int gs1_encoder_getBufferHeight(gs1_encoder *ctx);


/**
 * @brief Return the output buffer represented as an array of strings
 *
 * Example output:
 *
 * \code
 * {
 *     "                             ",
 *     "                             ",
 *     "                             ",
 *     "                             ",
 *     "    XXXXXXX  X XX XXXXXXX    ",
 *     "    X     X  X  X X     X    ",
 *     "    X XXX X XXX X X XXX X    ",
 *     "    X XXX X X     X XXX X    ",
 *     "    X XXX X XXXXX X XXX X    ",
 *     "    X     X XX  X X     X    ",
 *     "    XXXXXXX X X X XXXXXXX    ",
 *     "            X XXX            ",
 *     "    X XXXXX  XX X XXXXX      ",
 *     "      XX X X X  X  X  X      ",
 *     "    XXXX XXX  XX X  X  X     ",
 *     "     XXX   X X     XXXXX     ",
 *     "      X   XX  XX X  X X      ",
 *     "            X XXXXXX  X      ",
 *     "    XXXXXXX   X X XX X X     ",
 *     "    X     X X  XXXX X X X    ",
 *     "    X XXX X X X X  X X X     ",
 *     "    X XXX X XXX X   X X      ",
 *     "    X XXX X XXXX X X XX      ",
 *     "    X     X  XX    XX X      ",
 *     "    XXXXXXX XX X X X X X     ",
 *     "                             ",
 *     "                             ",
 *     "                             ",
 *     "                             "
 * }
 * \endcode
 *
 * \note
 * The storage for the string array is managed by the library and must not be
 * freed.
 *
 * @see gs1_encoder_getBuffer()
 *
 * @param [in,out] ctx ::gs1_encoder context
 * @param [out] strings the value of the given pointer is rewritten to point to an array of strings
 * @return the number of rows in the returned array of strings
 */
GS1_ENCODERS_API size_t gs1_encoder_getBufferStrings(gs1_encoder *ctx, char ***strings);


/**
 *  @brief Destroy a ::gs1_encoder instance.
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
