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

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "enc-private.h"
#include "dm.h"
#include "mtx.h"
#include "driver.h"


#define METRIC(r, c, rh, rv, cw, bl)		\
	{ .rows = r, .cols = c,			\
	  .regh = rh, .regv = rv,		\
	  .rscw = cw, .rsbl = bl,		\
	}


struct metric {
	uint8_t rows;
	uint8_t cols;
	uint8_t regh;
	uint8_t regv;
	uint16_t rscw;
	uint8_t rsbl;
};


static const struct metric metrics[30] = {
	//     rows  cols  regh  regv  rscw  rsbl
	METRIC(  10,   10,    1,    1,    5,    1),
	METRIC(  12,   12,    1,    1,    7,    1),
	METRIC(  14,   14,    1,    1,   10,    1),
	METRIC(  16,   16,    1,    1,   12,    1),
	METRIC(  18,   18,    1,    1,   14,    1),
	METRIC(  20,   20,    1,    1,   18,    1),
	METRIC(  22,   22,    1,    1,   20,    1),
	METRIC(  24,   24,    1,    1,   24,    1),
	METRIC(  26,   26,    1,    1,   28,    1),
	METRIC(  32,   32,    2,    2,   36,    1),
	METRIC(  36,   36,    2,    2,   42,    1),
	METRIC(  40,   40,    2,    2,   48,    1),
	METRIC(  44,   44,    2,    2,   56,    1),
	METRIC(  48,   48,    2,    2,   68,    1),
	METRIC(  52,   52,    2,    2,   84,    2),
	METRIC(  64,   64,    4,    4,  112,    2),
	METRIC(  72,   72,    4,    4,  144,    4),
	METRIC(  80,   80,    4,    4,  192,    4),
	METRIC(  88,   88,    4,    4,  224,    4),
	METRIC(  96,   96,    4,    4,  272,    4),
	METRIC( 104,  104,    4,    4,  336,    6),
	METRIC( 120,  120,    6,    6,  408,    6),
	METRIC( 132,  132,    6,    6,  496,    8),
	METRIC( 144,  144,    6,    6,  620,   10),
	METRIC(   8,   18,    1,    1,    7,    1),
	METRIC(   8,   32,    1,    2,   11,    1),
	METRIC(  12,   26,    1,    1,   14,    1),
	METRIC(  12,   36,    1,    2,   18,    1),
	METRIC(  16,   36,    1,    2,   24,    1),
	METRIC(  16,   48,    1,    2,   28,    1),
};


static const uint8_t rslog[256] = {
          0,    // undefined
             255,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
          4, 195,  39, 114, 227, 106, 134,  28, 243, 140,  44,  23,  55, 118, 211, 234,
          5, 219, 196,  96,  40, 222, 115, 103, 228,  78, 107, 125, 135,   8,  29, 162,
        244, 186, 141, 180,  45,  99,  24,  49,  56,  13, 119, 153, 212, 199, 235,  91,
          6,  76, 220, 217, 197,  11,  97, 184,  41,  36, 223, 253, 116, 138, 104, 193,
        229,  86,  79, 171, 108, 165, 126, 145, 136,  34,   9,  74,  30,  32, 163,  84,
        245, 173, 187, 204, 142,  81, 181, 190,  46,  88, 100, 159,  25, 231,  50, 207,
         57, 147,  14,  67, 120, 128, 154, 248, 213, 167, 200,  63, 236, 110,  92, 176,
          7, 161,  77, 124, 221, 102, 218,  95, 198,  90,  12, 152,  98,  48, 185, 179,
         42, 209,  37, 132, 224,  52, 254, 239, 117, 233, 139,  22, 105,  27, 194, 113,
        230, 206,  87, 158,  80, 189, 172, 203, 109, 175, 166,  62, 127, 247, 146,  66,
        137, 192,  35, 252,  10, 183,  75, 216,  31,  83,  33,  73, 164, 144,  85, 170,
        246,  65, 174,  61, 188, 202, 205, 157, 143, 169,  82,  72, 182, 215, 191, 251,
         47, 178,  89, 151, 101,  94, 160, 123,  26, 112, 232,  21,  51, 238, 208, 131,
         58,  69, 148,  18,  15,  16,  68,  17, 121, 149, 129,  19, 155,  59, 249,  70,
        214, 250, 168,  71, 201, 156,  64,  60, 237, 130, 111,  20,  93, 122, 177, 150,
};


static const uint8_t rsalog[256] = {
          1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
        229, 231, 227, 235, 251, 219, 155,  27,  54, 108, 216, 157,  23,  46,  92, 184,
         93, 186,  89, 178,  73, 146,   9,  18,  36,  72, 144,  13,  26,  52, 104, 208,
        141,  55, 110, 220, 149,   7,  14,  28,  56, 112, 224, 237, 247, 195, 171, 123,
        246, 193, 175, 115, 230, 225, 239, 243, 203, 187,  91, 182,  65, 130,  41,  82,
        164, 101, 202, 185,  95, 190,  81, 162, 105, 210, 137,  63, 126, 252, 213, 135,
         35,  70, 140,  53, 106, 212, 133,  39,  78, 156,  21,  42,  84, 168, 125, 250,
        217, 159,  19,  38,  76, 152,  29,  58, 116, 232, 253, 215, 131,  43,  86, 172,
        117, 234, 249, 223, 147,  11,  22,  44,  88, 176,  77, 154,  25,  50, 100, 200,
        189,  87, 174, 113, 226, 233, 255, 211, 139,  59, 118, 236, 245, 199, 163, 107,
        214, 129,  47,  94, 188,  85, 170, 121, 242, 201, 191,  83, 166,  97, 194, 169,
        127, 254, 209, 143,  51, 102, 204, 181,  71, 142,  49,  98, 196, 165, 103, 206,
        177,  79, 158,  17,  34,  68, 136,  61, 122, 244, 197, 167,  99, 198, 161, 111,
        222, 145,  15,  30,  60, 120, 240, 205, 183,  67, 134,  33,  66, 132,  37,  74,
        148,   5,  10,  20,  40,  80, 160, 109, 218, 153,  31,  62, 124, 248, 221, 151,
          3,   6,  12,  24,  48,  96, 192, 173, 119, 238, 241, 207, 179,  75, 150,   1,
};


// Reed Solomon product in the field
static inline uint8_t rsProd(uint8_t a, uint8_t b) {
        return a && b ? rsalog[ (rslog[a] + rslog[b]) % 255 ] : 0;
}


// Generate Reed Solomon coefficients
static void rsGenerateCoeffs(int size, uint8_t *coeffs) {

        int i, j;

        coeffs[0] = 1;
        for (i = 1; i <= size; i++) {
                coeffs[i] = coeffs[i-1];
                for (j = i-1; j > 0; j--)
                        coeffs[j] = coeffs[j-1] ^ rsProd(coeffs[j], rsalog[i]);
                coeffs[0] = rsProd(coeffs[0], rsalog[i]);
        }

}


static void rsEncode(uint8_t* datcws, int datlen, uint8_t* ecccws, int ecclen, uint8_t* coeffs) {

        int i, j;
        uint8_t tmp[MAX_DM_DAT_CWS_PER_BLK + MAX_DM_ECC_CWS_PER_BLK] = { 0 };

        memcpy(tmp, datcws, (size_t)datlen);

        for (i = 0; i < datlen; i++)
                for (j = 0; j < ecclen; j++)
                        tmp[i+j+1] = rsProd(coeffs[ecclen-j-1], tmp[i]) ^ tmp[i+j+1];

        memcpy(ecccws, tmp + datlen, (size_t)ecclen);

}

static void createCodewords(gs1_encoder *ctx, uint8_t *string, uint8_t cws[MAX_DM_CWS], uint16_t* cwslen) {

(void)ctx;
(void)string;
(void)cws;
(void)cwslen;



uint8_t tmp[] = {
230, 89, 209, 109, 71, 202, 188, 92, 146, 121, 17, 109, 71, 202, 188, 92, 146,
121, 17, 121, 17, 109, 89, 254, 69, 71, 129, 254, 150, 45, 195, 90, 240, 136,
31, 181, 76, 226, 121, 17, 167, 62, 212, 107, 3, 153, 48, 198, 93, 243, 139,
34, 184, 79, 229, 124, 20, 170, 65, 215, 110, 6, 156, 51, 201, 96, 246, 142,
37, 187, 82, 232, 127, 23, 173, 68, 218, 113, 9, 159, 54, 204, 99, 249, 145,
40, 190, 85, 235, 131, 26, 176, 71, 221, 116, 12, 162, 57, 207, 102, 252, 148,
43, 193, 88, 238, 134, 29, 179, 74, 224, 119, 15, 165, 60, 210, 105, 1, 151,
46, 196, 91, 241, 137, 32, 182, 77, 227, 122, 18, 168, 63, 213, 108, 4, 154,
49, 199, 94, 244, 140, 35, 185, 80, 230, 125, 21, 171, 66, 216, 111, 7, 157,
52, 202, 97, 247, 143, 38, 188, 83, 233, 128, 24, 174, 69, 219, 114, 10, 160,
55, 205, 100, 250, 146, 41, 191, 86, 236, 132, 27, 177, 72, 222, 117, 13, 163,
58, 208, 103, 253, 149, 44, 194, 89, 239, 135, 30, 180, 75, 225, 120, 16, 166,
61, 211, 106, 2, 152, 47, 197, 92, 242, 138, 33, 183, 78, 228, 123, 19, 169,
64, 214, 109, 5, 155, 50, 200, 95, 245, 141, 36, 186, 81, 231, 126, 22, 172,
67, 217, 112, 8, 158, 53, 203, 98, 248, 144, 39, 189, 84, 234, 130, 25, 175,
70, 220, 115, 11, 161, 56, 206, 101, 251, 147, 42, 192, 87, 237, 133, 28, 178,
73, 223, 118, 14, 164, 59, 209, 104, 254, 150, 45, 195, 90, 240, 136, 31, 181,
76, 226, 121, 17, 167, 62, 212, 107, 3, 153, 48, 198, 93, 243, 139, 34, 184,
79, 229, 124, 20, 170, 65, 215, 110, 6, 156, 51, 201, 96, 246, 142, 37, 187,
82, 232, 127, 23, 173, 68, 218, 113, 9, 159, 54, 204, 99, 249, 145, 40, 190,
85, 235, 131, 26, 176, 71, 221, 116, 12, 162, 57, 207, 102, 252, 148, 43, 193,
88, 238, 134, 29, 179, 74, 224, 119, 15, 165, 60, 210, 105, 1, 151, 46, 196,
91, 241, 137, 32, 182, 77, 227, 122, 18, 168, 63, 213, 108, 4, 154, 49, 199,
94, 244, 140, 35, 185, 80, 230, 125, 21, 171, 66, 216, 111, 7, 157, 52, 202,
97, 247, 143, 38, 188, 83, 233, 128, 24, 174, 69, 219, 114, 10, 160, 55, 205,
100, 250, 146, 41, 191, 86, 236, 132, 27, 177, 72, 222, 117, 13, 163, 58, 208,
103, 253, 149, 44, 194, 89, 239, 135, 30, 180, 75, 225, 120, 16, 166, 61, 211,
106, 2, 152, 47, 197, 92, 242, 138, 33, 183, 78, 228, 123, 19, 169, 64, 214,
109, 5, 155, 50, 200, 95, 245, 141, 36, 186, 81, 231, 126, 22, 172, 67, 217,
112, 8, 158, 53, 203, 98, 248, 144, 39, 189, 84, 234, 130, 25, 175, 70, 220,
115, 11, 161, 56, 206, 101, 251, 147, 42, 192, 87, 237, 133, 28, 178, 73, 223,
118, 14, 164, 59, 209, 104, 254, 150, 45, 195, 90, 240, 136, 31, 181, 76, 226,
121, 17, 167, 62, 212, 107, 3, 153, 48, 198, 93, 243, 139, 34, 184, 79, 229,
124, 20, 170, 65, 215, 110, 6, 156, 51, 201, 96, 246, 142, 37, 187, 82, 232,
127, 23, 173, 68, 218, 113, 9, 159, 54, 204, 99, 249, 145, 40, 190, 85, 235,
131, 26, 176, 71, 221, 116, 12, 162, 57, 207, 102, 252, 148, 43, 193, 88, 238,
134, 29, 179, 74, 224, 119, 15, 165, 60, 210, 105, 1, 151, 46, 196, 91, 241,
137, 32, 182, 77, 227, 122, 18, 168, 63, 213, 108, 4, 154, 49, 199, 94, 244,
140, 35, 185, 80, 230, 125, 21, 171, 66, 216, 111, 7, 157, 52, 202, 97, 247,
143, 38, 188, 83, 233, 128, 24, 174, 69, 219, 114, 10, 160, 55, 205, 100, 250,
146, 41, 191, 86, 236, 132, 27, 177, 72, 222, 117, 13, 163, 58, 208, 103, 253,
149, 44, 194, 89, 239, 135, 30, 180, 75, 225, 120, 16, 166, 61, 211, 106, 2,
152, 47, 197, 92, 242, 138, 33, 183, 78, 228, 123, 19, 169, 64, 214, 109, 5,
155, 50, 200, 95, 245, 141, 36, 186, 81, 231, 126, 22, 172, 67, 217, 112, 8,
158, 53, 203, 98, 248, 144, 39, 189, 84, 234, 130, 25, 175, 70, 220, 115, 11,
161, 56, 206, 101, 251, 147, 42, 192, 87, 237, 133, 28, 178, 73, 223, 118, 14,
164, 59, 209, 104, 254, 150, 45, 195, 90, 240, 136, 31, 181, 76, 226, 121, 17,
167, 62, 212, 107, 3, 153, 48, 198, 93, 243, 139, 34, 184, 79, 229, 124, 20,
170, 65, 215, 110, 6, 156, 51, 201, 96, 246, 142, 37, 187, 82, 232, 127, 23,
173, 68, 218, 113, 9, 159, 54, 204, 99, 249, 145, 40, 190, 85, 235, 131, 26,
176, 71, 221, 116, 12, 162, 57, 207, 102, 252, 148, 43, 193, 88, 238, 134, 29,
179, 74, 224, 119, 15, 165, 60, 210, 105, 1, 151, 46, 196, 91, 241, 137, 32,
182, 77, 227, 122, 18, 168, 63, 213, 108, 4, 154, 49, 199, 94, 244, 140, 35,
185, 80, 230, 125, 21, 171, 66, 216, 111, 7, 157, 52, 202, 97, 247, 143, 38,
188, 83, 233, 128, 24, 174, 69, 219, 114, 10, 160, 55, 205, 100, 250, 146, 41,
191, 86, 236, 132, 27, 177, 72, 222, 117, 13, 163, 58, 208, 103, 253, 149, 44,
194, 89, 239, 135, 30, 180, 75, 225, 120, 16, 166, 61, 211, 106, 2, 152, 47,
197, 92, 242, 138, 33, 183, 78, 228, 123, 19, 169, 64, 214, 109, 5, 155, 50,
200, 95, 245, 141, 36, 186, 81, 231, 126, 22, 172, 67, 217, 112, 8, 158, 53,
203, 98, 248, 144, 39, 189, 84, 234, 130, 25, 175, 70, 220, 115, 11, 161, 56,
206, 101, 251, 147, 42, 192, 87, 237, 133, 28, 178, 73, 223, 118, 14, 164, 59,
209, 104, 254, 150, 45, 195, 90, 240, 136, 31, 181, 76, 226, 121, 17, 167, 62,
212, 107, 3, 153, 48, 198, 93, 243, 139, 34, 184, 79, 229, 124, 20, 170, 65,
215, 110, 6, 156, 51, 201, 96, 246, 142, 37, 187, 82, 232, 127, 23, 173, 68,
218, 113, 9, 159, 54, 204, 99, 249, 145, 40, 190, 85, 235, 131, 26, 176, 71,
221, 116, 12, 162, 57, 207, 102, 252, 148, 43, 193, 88, 238, 134, 29, 179, 74,
224, 119, 15, 165, 60, 210, 105, 1, 151, 46, 196, 91, 241, 137, 32, 182, 77,
227, 122, 18, 168, 63, 213, 108, 4, 154, 49, 199, 94, 244, 140, 35, 185, 80,
230, 125, 21, 171, 66, 216, 111, 7, 157, 52, 202, 97, 247, 143, 38, 188, 83,
233, 128, 24, 174, 69, 219, 114, 10, 160, 55, 205, 100, 250, 146, 41, 191, 86,
236, 132, 27, 177, 72, 222, 117, 13, 163, 58, 208, 103, 253, 149, 44, 194, 89,
239, 135, 30, 180, 75, 225, 120, 16, 166, 61, 211, 106, 2, 152, 47, 197, 92,
242, 138, 33, 183, 78, 228, 123, 19, 169, 64, 214, 109, 5, 155, 50, 200, 95,
245, 141, 36, 186, 81, 231, 126, 22, 172, 67, 217, 112, 8, 158, 53, 203, 98,
248, 144, 39, 189, 84, 234, 130, 25, 175, 70, 220, 115, 11, 161, 56, 206, 101,
251, 147, 42, 192, 87, 237, 133, 28, 178, 73, 223, 118, 14, 164, 59, 209, 104,
254, 150, 45, 195, 90, 240, 136, 31, 181, 76, 226, 121, 17, 167, 62, 212, 107,
3, 153, 48, 198, 93, 243, 139, 34, 184, 79, 229, 124, 20, 170, 65, 215, 110, 6,
156, 51, 201, 96, 246, 142, 37, 187, 82, 232, 127, 23, 173, 68, 218, 113, 9,
159, 54, 204, 99, 249, 145, 40, 190, 85, 235, 131, 26, 176, 71, 221, 116, 12,
162, 57, 207, 102, 252, 148, 43, 193, 88, 238, 134, 29, 179, 74, 224, 119, 15,
165, 60, 210, 105, 1, 151, 46, 196, 91, 241, 137, 32, 182, 77, 227, 122, 18,
168, 63, 213, 108, 4, 154, 49, 199, 94, 244, 140, 35, 185, 80, 230, 125, 21,
171, 66, 216, 111, 7, 157, 52, 202, 97, 247, 143, 38, 188, 83, 233, 128, 24,
174, 69, 219, 114, 10, 160, 55, 205, 100, 250, 146, 41, 191, 86, 236, 132, 27,
177, 72, 222, 117, 13, 163, 58, 208, 103, 253, 149, 44, 194, 89, 239, 135, 30,
180, 75, 225, 120, 16, 166, 61, 211, 106, 2, 152, 47, 197, 92, 242, 138, 33,
183, 78, 228, 123, 19, 169, 64, 214, 109, 5, 155, 50, 200, 95, 245, 141, 36,
186, 81, 231, 126, 22, 172, 67, 217, 112, 8, 158, 53, 203, 98, 248, 144, 39,
189, 84, 234, 130, 25, 175, 70, 220, 115, 11, 161, 56, 206, 101, 251, 147, 42,
192, 87, 237, 133, 28, 178, 73, 223, 118, 14, 164, 59, 209, 104, 254, 150, 45,
195, 90, 240, 136, 31, 181, 76, 226, 121, 17};


memcpy(cws, tmp, 1558);

*cwslen = 1558;

}


static const struct metric* selectVersion(gs1_encoder *ctx, uint16_t cwslen) {

(void)ctx;
(void)cwslen;

	return &metrics[23];  // TODO 144x144
}


static void finaliseCodewords(gs1_encoder *ctx, uint8_t *cws, uint16_t *cwslen, const struct metric *m) {

	uint8_t tmpcws[MAX_DM_DAT_CWS_PER_BLK+MAX_DM_ECC_CWS_PER_BLK];
	uint8_t coeffs[MAX_DM_ECC_CWS_PER_BLK+1];

	int ncws, rscw, rsbl;

	int i, j;
	uint8_t *p;

	(void)ctx;

	ncws = *cwslen;		// Data codewords
	rscw = m->rscw;		// Error correction codewords
	rsbl = m->rsbl;		// Error correction blocks

	// Generate coefficients
	rsGenerateCoeffs(rscw/rsbl, coeffs);

	// Error correction for interleaved blocks of codewords
	for (i = 0; i < rsbl; i++) {

		p = tmpcws;
		for (j = i; j < ncws; j += rsbl)
			*p++ = cws[j];

		rsEncode(tmpcws, (int)(p-tmpcws), p, rscw/rsbl, coeffs);

		int offset = m->rscw == 620 ? (i<8 ? 2:-8) : 0;
		for (j = i; j < rscw; j += rsbl)
			cws[ncws+j+offset] = *p++;

	}

}


#define putTimingModule(cx,rx,b) do {							\
	gs1_mtxPutModule(mtx, m->cols + 2*DM_QZ,					\
			 DM_QZ + cx, DM_QZ + rx,					\
			 b);								\
} while(0)


// Place a data module, handling wrapping and QZ, jumping occtures and mark it
// as reserved
#define putModule(cx,rx,b) do {								\
	int cc = cx; int rr = rx;							\
	if (rr < 0)      { rr += mrows; cc += 4-(mrows+4)%8; }				\
	if (cc < 0)      { cc += mcols; rr += 4-(mcols+4)%8; }				\
	if (rr >= mrows) { rr -= mrows;                      }				\
	gs1_mtxPutModule(occ, mcols, cc, rr, 1);					\
	gs1_mtxPutModule(mtx, m->cols + 2*DM_QZ,					\
			 DM_QZ + cc + 2*(cc/(mcols/m->regv)) + 1,			\
			 DM_QZ + rr + 2*(rr/(mrows/m->regh)) + 1,			\
			 b);								\
} while(0)


#define plotCodeword(c1,r1,c2,r2,c3,r3,c4,r4,c5,r5,c6,r6,c7,r7,c8,r8) do {		\
	putModule(c1, r1, *cws >>7 & 1);						\
	putModule(c2, r2, *cws >>6 & 1);						\
	putModule(c3, r3, *cws >>5 & 1);						\
	putModule(c4, r4, *cws >>4 & 1);						\
	putModule(c5, r5, *cws >>3 & 1);						\
	putModule(c6, r6, *cws >>2 & 1);						\
	putModule(c7, r7, *cws >>1 & 1);						\
	putModule(c8, r8, *cws >>0 & 1);						\
	cws++;										\
} while(0)


#define plotCodewordCorner(c1,r1,c2,r2,c3,r3,c4,r4,c5,r5,c6,r6,c7,r7,c8,r8) do {	\
	plotCodeword(c1 >= 0 ? c1 : c1+mcols, r1 >= 0 ? r1 : r1+mrows,			\
		     c2 >= 0 ? c2 : c2+mcols, r2 >= 0 ? r2 : r2+mrows,			\
		     c3 >= 0 ? c3 : c3+mcols, r3 >= 0 ? r3 : r3+mrows,			\
		     c4 >= 0 ? c4 : c4+mcols, r4 >= 0 ? r4 : r4+mrows,			\
		     c5 >= 0 ? c5 : c5+mcols, r5 >= 0 ? r5 : r5+mrows,			\
		     c6 >= 0 ? c6 : c6+mcols, r6 >= 0 ? r6 : r6+mrows,			\
		     c7 >= 0 ? c7 : c7+mcols, r7 >= 0 ? r7 : r7+mrows,			\
		     c8 >= 0 ? c8 : c8+mcols, r8 >= 0 ? r8 : r8+mrows);			\
} while(0)


static void createMatrix(gs1_encoder *ctx, uint8_t *mtx, uint8_t *cws, const struct metric *m) {

	uint8_t occ[MAX_DM_BYTES] = { 0 };  // Matrix to indicate occupied positions
	int i, j;
	int mrows, mcols;

	(void)ctx;

	mrows = m->rows - 2*m->regh;  // Rows excluding timing patterns
	mcols = m->cols - 2*m->regv;  // Columns excluding timing patterns

	// Plot timing patterns
	for (i = 0; i < m->cols+1; i += mcols/m->regv + 2) {
		for (j = 0; j < m->rows; j++) {
			if (i > 0)
				putTimingModule(i-1, j, (uint8_t)(j%2));
			if (i < m->cols)
				putTimingModule(i, j, 1);
		}
	}
	for (j = 0; j < m->rows+1; j += mrows/m->regh + 2) {
		for (i = 0; i < m->cols; i++) {
			if (j > 0)
				putTimingModule(i, j-1, 1);
			if (j < m->rows)
				putTimingModule(i, j, (uint8_t)(1-i%2));
		}
	}


	// Place the modules between the timing patterns
	i = 0; j = 4;

	do {

		if (i == 0 && j == mrows)
			plotCodewordCorner(
				0,-1,	1,-1,	2,-1,	/**/
				/***********************/  /****************/
				/***********************/  /****************/
							/**/	-2,0,	-1,0,
							/**/	-1,1,
							/**/	-1,2,
							/**/	-1,3
			);
		if (i == 0 && j == mrows-2 && mcols%4 != 0)
			plotCodewordCorner(
				0,-3,	/**/
				0,-2,	/**/
				0,-1,	/**/
				/*******/  /********************************/
				/*******/  /********************************/
					/**/	-4,0,	-3,0,	-2,0,	-1,0,
					/**/				-1,1
			);
		if (i == 0 && j == mrows-2 && mcols%8 == 4)
			plotCodewordCorner(
				0,-3,	/**/
				0,-2,	/**/
				0,-1,	/**/
				/*******/  /****************/
				/*******/  /****************/
					/**/	-2,0,	-1,0,
					/**/		-1,1,
					/**/		-1,2,
					/**/		-1,3
			);
		if (i == 2 && j == mrows+4 && mcols%8 == 0)
			plotCodewordCorner(
				0,-1,	/**/			-1,-1,
				/*******/  /*************************/
				/*******/  /*************************/
					/**/	-3,0,	-2,0,	-1,0,
					/**/	-3,1,	-2,1,	-1,1
			);

		// Sweep upwards
		do {
			if (i >= 0 && j < mrows && !gs1_mtxGetModule(occ, mcols, i, j)) {
				plotCodeword(
					i-2,j-2,  i-1,j-2,
					i-2,j-1,  i-1,j-1,  i-0,j-1,
					i-2,j-0,  i-1,j-0,  i-0,j-0
				);
			}
			i+=2; j-=2;
		} while (i < mcols && j >= 0);
		i+=3; j++;

		// Sweep downwards
		do {
			if (i < mcols && j >= 0 && !gs1_mtxGetModule(occ, mcols, i, j))
				plotCodeword(
					i-2,j-2,  i-1,j-2,
					i-2,j-1,  i-1,j-1,  i-0,j-1,
					i-2,j-0,  i-1,j-0,  i-0,j-0
				);
			i-=2; j+=2;
		} while (i >= 0 && j < mrows);
		i++; j+=3;

	} while (i < mcols || j < mrows);


	// Set checker pattern if required
	if (gs1_mtxGetModule(occ, mcols, mrows-1, mcols-1) == 0) {
		putModule(mrows-2, mcols-2, 1);
		putModule(mrows-1, mcols-2, 0);
		putModule(mrows-2, mcols-1, 0);
		putModule(mrows-1, mcols-1, 1);
	}

}


static int DMenc(gs1_encoder *ctx, uint8_t string[], struct patternLength *pats) {

	uint8_t mtx[MAX_DM_BYTES] = { 0 };
	uint8_t cws[MAX_DM_CWS] = { 0 };
	uint16_t cwslen = 0;
	const struct metric *m;

	(void)ctx;
	(void)string;

	createCodewords(ctx, string, cws, &cwslen);

	m = selectVersion(ctx, cwslen);
	if (!m) {
		strcpy(ctx->errMsg, "No suitable symbol found");
		ctx->errFlag = true;
		return 0;
	}

	finaliseCodewords(ctx, cws, &cwslen, m);
	createMatrix(ctx, mtx, cws, m);

	gs1_mtxToPatterns(mtx, m->cols + 2*DM_QZ, m->rows + 2*DM_QZ, pats);

	return m->rows + 2*DM_QZ;

}


void gs1_DM(gs1_encoder *ctx) {

	struct sPrints prints;
	struct patternLength *pats;
	char* dataStr = ctx->dataStr;
	int rows, cols, i;

	pats = malloc(MAX_DM_ROWS * sizeof(struct patternLength));
	if (!pats) {
		strcpy(ctx->errMsg, "Out of memory allocating patterns");
		ctx->errFlag = true;
		return;
	}

	if (!(rows = DMenc(ctx, (uint8_t*)dataStr, pats)) || ctx->errFlag)
		goto out;

#if PRNT
	{
		int j;
		printf("\n%s\n", dataStr);
		printf("\n");
		for (i = 0; i < rows; i++) {
			for (j = 0; j < pats[i].length; j++) {
				printf("%d", pats[i].pattern[j]);
			}
			printf("\n");
		}
		printf("\n");
	}
#endif

	cols = 0;
	for (i = 0; i < pats[0].length; i++)
		cols += pats[0].pattern[i];

	gs1_driverInit(ctx, ctx->pixMult*cols, ctx->pixMult*rows);

	ctx->line1 = true; // so first line is not Y undercut
	prints.height = ctx->pixMult;
	prints.guards = false;
	prints.leftPad = 0;
	prints.rightPad = 0;
	prints.reverse = false;

	for (i = 0; i < rows; i++) {
		prints.elmCnt = pats[i].length;
		prints.pattern = pats[i].pattern;
		prints.whtFirst = pats[i].whtFirst;
		gs1_driverAddRow(ctx, &prints);
	}

	gs1_driverFinalise(ctx);

out:

	free(pats);

	return;

}


#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"

#include "gs1encoders-test.h"


void test_dm_DM_encode(void) {

	char** expect;

	gs1_encoder* ctx = gs1_encoder_init();

	expect = (char*[]){
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
"01010101010101010101",
"10101010101010101010",
NULL
	};
	TEST_CHECK(test_encode(ctx, gs1_encoder_sDM, "1501234567890", expect));

	test_print_strings(ctx);

	gs1_encoder_free(ctx);

}


#endif  /* UNIT_TESTS */
