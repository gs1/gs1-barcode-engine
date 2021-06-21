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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gs1encoders.h"
#include "enc-private.h"
#include "debug.h"
#include "gs1.h"


// AI prefixes that are defined as not requiring termination by an FNC1 character
static const char* fixedAIprefixes[22] = {
	"00", "01", "02",
	"03", "04",
	"11", "12", "13", "14", "15", "16", "17", "18", "19",
	"20",
	// "23",	// No longer defined as fixed length
	"31", "32", "33", "34", "35", "36",
	"41"
};


/*
 *  Set of 82 characters valid within type "X" AIs
 *
 */
static const char* cset82 = "!\"%&'()*+,-./0123456789:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";


/* "Linter" functions
 *
 * Used to validate AI components
 *
 */

static bool lint_cset82(gs1_encoder *ctx, const struct aiEntry *entry, char *val) {
	DEBUG_PRINT("      cset82...");
	if (strspn(val, cset82) != strlen(val)) {
		sprintf(ctx->errMsg, "AI (%s): Incorrect CSET 82 character", entry->ai);
		ctx->errFlag = true;
		return false;
	}
	DEBUG_PRINT(" success\n");
	return true;
}


static bool lint_csetNumeric(gs1_encoder *ctx, const struct aiEntry *entry, char *val) {
	DEBUG_PRINT("      csetNumeric...");
	if (!gs1_allDigits((uint8_t*)val)) {
		sprintf(ctx->errMsg, "AI (%s): Illegal non-digit character", entry->ai);
		ctx->errFlag = true;
		return false;
	}
	DEBUG_PRINT(" success\n");
	return true;
}


static bool lint_csum(gs1_encoder *ctx, const struct aiEntry *entry, char *val) {
	DEBUG_PRINT("      csum...");
	if (!gs1_validateParity((uint8_t*)val)) {
		DEBUG_PRINT(" failed\n");
		sprintf(ctx->errMsg, "AI (%s): Incorrect check digit", entry->ai);
		ctx->errFlag = true;
		return false;
	};
	DEBUG_PRINT(" success\n");
	return true;
}


#define FNC1 true
#define NO_FNC1 false


#define AI_VA(a, f, c1,mn1,mx1,l01, c2,mn2,mx2,l02, c3,mn3,mx3,l03, c4,mn4,mx4,l04, c5,mn5,mx5,l05, t) {	\
		.ai = a,											\
		.fnc1 = f,											\
		.parts = {											\
			{ .cset = cset_##c1, .min = mn1, .max = mx1, .linters[0] = lint_##l01 },		\
			{ .cset = cset_##c2, .min = mn2, .max = mx2, .linters[0] = lint_##l02 },		\
			{ .cset = cset_##c3, .min = mn3, .max = mx3, .linters[0] = lint_##l03 },		\
			{ .cset = cset_##c4, .min = mn4, .max = mx4, .linters[0] = lint_##l04 },		\
			{ .cset = cset_##c5, .min = mn5, .max = mx5, .linters[0] = lint_##l05 },		\
		},												\
		.title = t,											\
	}
#define PASS_ON(...) __VA_ARGS__
#define AI(...) PASS_ON(AI_VA(__VA_ARGS__))
#define cset_0 0
#define lint__ 0
#define __ 0,0,0,_

static const struct aiEntry ai_table[] = {
	AI( "00"  , NO_FNC1, N,18,18,csum, __, __, __, __,                "SSCC"                      ),
	AI( "01"  , NO_FNC1, N,14,14,csum, __, __, __, __,                "GTIN"                      ),
	AI( "02"  , NO_FNC1, N,14,14,csum, __, __, __, __,                "CONTENT"                   ),
	AI( "10"  , FNC1   , X,1,20,_, __, __, __, __,                    "BATCH/LOT"                 ),
	AI( "11"  , NO_FNC1, N,6,6,_, __, __, __, __,                     "PROD DATE"                 ),
	AI( "12"  , NO_FNC1, N,6,6,_, __, __, __, __,                     "DUE DATE"                  ),
	AI( "13"  , NO_FNC1, N,6,6,_, __, __, __, __,                     "PACK DATE"                 ),
	AI( "15"  , NO_FNC1, N,6,6,_, __, __, __, __,                     "BEST BEFORE or BEST BY"    ),
	AI( "16"  , NO_FNC1, N,6,6,_, __, __, __, __,                     "SELL BY"                   ),
	AI( "17"  , NO_FNC1, N,6,6,_, __, __, __, __,                     "USE BY or EXPIRY"          ),
	AI( "20"  , NO_FNC1, N,2,2,_, __, __, __, __,                     "VARIANT"                   ),
	AI( "21"  , FNC1   , X,1,20,_, __, __, __, __,                    "SERIAL"                    ),
	AI( "22"  , FNC1   , X,1,20,_, __, __, __, __,                    "CPV"                       ),
	AI( "235" , FNC1   , X,1,28,_, __, __, __, __,                    "TPX"                       ),
	AI( "240" , FNC1   , X,1,30,_, __, __, __, __,                    "ADDITIONAL ID"             ),
	AI( "241" , FNC1   , X,1,30,_, __, __, __, __,                    "CUST. PART NO."            ),
	AI( "242" , FNC1   , N,1,6,_, __, __, __, __,                     "MTO VARIANT"               ),
	AI( "243" , FNC1   , X,1,20,_, __, __, __, __,                    "PCN"                       ),
	AI( "250" , FNC1   , X,1,30,_, __, __, __, __,                    "SECONDARY SERIAL"          ),
	AI( "251" , FNC1   , X,1,30,_, __, __, __, __,                    "REF. TO SOURCE"            ),
	AI( "253" , FNC1   , N,13,13,csum, X,0,17,_, __, __, __,          "GDTI"                      ),
	AI( "254" , FNC1   , X,1,20,_, __, __, __, __,                    "GLN EXTENSION COMPONENT"   ),
	AI( "255" , FNC1   , N,13,13,csum, N,0,12,_, __, __, __,          "GCN"                       ),
	AI( "30"  , FNC1   , N,1,8,_, __, __, __, __,                     "VAR. COUNT"                ),
	AI( "3100", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (kg)"           ),
	AI( "3101", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (kg)"           ),
	AI( "3102", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (kg)"           ),
	AI( "3103", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (kg)"           ),
	AI( "3104", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (kg)"           ),
	AI( "3105", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (kg)"           ),
	AI( "3110", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m)"                ),
	AI( "3111", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m)"                ),
	AI( "3112", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m)"                ),
	AI( "3113", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m)"                ),
	AI( "3114", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m)"                ),
	AI( "3115", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m)"                ),
	AI( "3120", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m)"                 ),
	AI( "3121", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m)"                 ),
	AI( "3122", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m)"                 ),
	AI( "3123", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m)"                 ),
	AI( "3124", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m)"                 ),
	AI( "3125", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m)"                 ),
	AI( "3130", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m)"                ),
	AI( "3131", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m)"                ),
	AI( "3132", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m)"                ),
	AI( "3133", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m)"                ),
	AI( "3134", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m)"                ),
	AI( "3135", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m)"                ),
	AI( "3140", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2)"                ),
	AI( "3141", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2)"                ),
	AI( "3142", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2)"                ),
	AI( "3143", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2)"                ),
	AI( "3144", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2)"                ),
	AI( "3145", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2)"                ),
	AI( "3150", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (l)"            ),
	AI( "3151", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (l)"            ),
	AI( "3152", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (l)"            ),
	AI( "3153", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (l)"            ),
	AI( "3154", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (l)"            ),
	AI( "3155", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (l)"            ),
	AI( "3160", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (m^3)"          ),
	AI( "3161", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (m^3)"          ),
	AI( "3162", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (m^3)"          ),
	AI( "3163", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (m^3)"          ),
	AI( "3164", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (m^3)"          ),
	AI( "3165", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (m^3)"          ),
	AI( "3200", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (lb)"           ),
	AI( "3201", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (lb)"           ),
	AI( "3202", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (lb)"           ),
	AI( "3203", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (lb)"           ),
	AI( "3204", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (lb)"           ),
	AI( "3205", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (lb)"           ),
	AI( "3210", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i)"                ),
	AI( "3211", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i)"                ),
	AI( "3212", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i)"                ),
	AI( "3213", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i)"                ),
	AI( "3214", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i)"                ),
	AI( "3215", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i)"                ),
	AI( "3220", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f)"                ),
	AI( "3221", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f)"                ),
	AI( "3222", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f)"                ),
	AI( "3223", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f)"                ),
	AI( "3224", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f)"                ),
	AI( "3225", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f)"                ),
	AI( "3230", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y)"                ),
	AI( "3231", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y)"                ),
	AI( "3232", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y)"                ),
	AI( "3233", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y)"                ),
	AI( "3234", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y)"                ),
	AI( "3235", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y)"                ),
	AI( "3240", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i)"                 ),
	AI( "3241", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i)"                 ),
	AI( "3242", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i)"                 ),
	AI( "3243", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i)"                 ),
	AI( "3244", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i)"                 ),
	AI( "3245", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i)"                 ),
	AI( "3250", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f)"                 ),
	AI( "3251", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f)"                 ),
	AI( "3252", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f)"                 ),
	AI( "3253", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f)"                 ),
	AI( "3254", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f)"                 ),
	AI( "3255", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f)"                 ),
	AI( "3260", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y)"                 ),
	AI( "3261", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y)"                 ),
	AI( "3262", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y)"                 ),
	AI( "3263", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y)"                 ),
	AI( "3264", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y)"                 ),
	AI( "3265", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y)"                 ),
	AI( "3270", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i)"                ),
	AI( "3271", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i)"                ),
	AI( "3272", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i)"                ),
	AI( "3273", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i)"                ),
	AI( "3274", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i)"                ),
	AI( "3275", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i)"                ),
	AI( "3280", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f)"                ),
	AI( "3281", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f)"                ),
	AI( "3282", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f)"                ),
	AI( "3283", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f)"                ),
	AI( "3284", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f)"                ),
	AI( "3285", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f)"                ),
	AI( "3290", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y)"                ),
	AI( "3291", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y)"                ),
	AI( "3292", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y)"                ),
	AI( "3293", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y)"                ),
	AI( "3294", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y)"                ),
	AI( "3295", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y)"                ),
	AI( "3300", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (kg)"         ),
	AI( "3301", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (kg)"         ),
	AI( "3302", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (kg)"         ),
	AI( "3303", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (kg)"         ),
	AI( "3304", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (kg)"         ),
	AI( "3305", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (kg)"         ),
	AI( "3310", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m), log"           ),
	AI( "3311", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m), log"           ),
	AI( "3312", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m), log"           ),
	AI( "3313", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m), log"           ),
	AI( "3314", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m), log"           ),
	AI( "3315", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (m), log"           ),
	AI( "3320", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m), log"            ),
	AI( "3321", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m), log"            ),
	AI( "3322", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m), log"            ),
	AI( "3323", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m), log"            ),
	AI( "3324", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m), log"            ),
	AI( "3325", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (m), log"            ),
	AI( "3330", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m), log"           ),
	AI( "3331", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m), log"           ),
	AI( "3332", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m), log"           ),
	AI( "3333", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m), log"           ),
	AI( "3334", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m), log"           ),
	AI( "3335", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (m), log"           ),
	AI( "3340", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2), log"           ),
	AI( "3341", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2), log"           ),
	AI( "3342", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2), log"           ),
	AI( "3343", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2), log"           ),
	AI( "3344", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2), log"           ),
	AI( "3345", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (m^2), log"           ),
	AI( "3350", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (l), log"           ),
	AI( "3351", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (l), log"           ),
	AI( "3352", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (l), log"           ),
	AI( "3353", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (l), log"           ),
	AI( "3354", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (l), log"           ),
	AI( "3355", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (l), log"           ),
	AI( "3360", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (m^3), log"         ),
	AI( "3361", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (m^3), log"         ),
	AI( "3362", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (m^3), log"         ),
	AI( "3363", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (m^3), log"         ),
	AI( "3364", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (m^3), log"         ),
	AI( "3365", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (m^3), log"         ),
	AI( "3370", NO_FNC1, N,6,6,_, __, __, __, __,                     "KG PER m^2"                ),
	AI( "3371", NO_FNC1, N,6,6,_, __, __, __, __,                     "KG PER m^2"                ),
	AI( "3372", NO_FNC1, N,6,6,_, __, __, __, __,                     "KG PER m^2"                ),
	AI( "3373", NO_FNC1, N,6,6,_, __, __, __, __,                     "KG PER m^2"                ),
	AI( "3374", NO_FNC1, N,6,6,_, __, __, __, __,                     "KG PER m^2"                ),
	AI( "3375", NO_FNC1, N,6,6,_, __, __, __, __,                     "KG PER m^2"                ),
	AI( "3400", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (lb)"         ),
	AI( "3401", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (lb)"         ),
	AI( "3402", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (lb)"         ),
	AI( "3403", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (lb)"         ),
	AI( "3404", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (lb)"         ),
	AI( "3405", NO_FNC1, N,6,6,_, __, __, __, __,                     "GROSS WEIGHT (lb)"         ),
	AI( "3410", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i), log"           ),
	AI( "3411", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i), log"           ),
	AI( "3412", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i), log"           ),
	AI( "3413", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i), log"           ),
	AI( "3414", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i), log"           ),
	AI( "3415", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (i), log"           ),
	AI( "3420", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f), log"           ),
	AI( "3421", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f), log"           ),
	AI( "3422", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f), log"           ),
	AI( "3423", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f), log"           ),
	AI( "3424", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f), log"           ),
	AI( "3425", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (f), log"           ),
	AI( "3430", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y), log"           ),
	AI( "3431", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y), log"           ),
	AI( "3432", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y), log"           ),
	AI( "3433", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y), log"           ),
	AI( "3434", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y), log"           ),
	AI( "3435", NO_FNC1, N,6,6,_, __, __, __, __,                     "LENGTH (y), log"           ),
	AI( "3440", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i), log"            ),
	AI( "3441", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i), log"            ),
	AI( "3442", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i), log"            ),
	AI( "3443", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i), log"            ),
	AI( "3444", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i), log"            ),
	AI( "3445", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (i), log"            ),
	AI( "3450", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f), log"            ),
	AI( "3451", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f), log"            ),
	AI( "3452", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f), log"            ),
	AI( "3453", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f), log"            ),
	AI( "3454", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f), log"            ),
	AI( "3455", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (f), log"            ),
	AI( "3460", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y), log"            ),
	AI( "3461", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y), log"            ),
	AI( "3462", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y), log"            ),
	AI( "3463", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y), log"            ),
	AI( "3464", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y), log"            ),
	AI( "3465", NO_FNC1, N,6,6,_, __, __, __, __,                     "WIDTH (y), log"            ),
	AI( "3470", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i), log"           ),
	AI( "3471", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i), log"           ),
	AI( "3472", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i), log"           ),
	AI( "3473", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i), log"           ),
	AI( "3474", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i), log"           ),
	AI( "3475", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (i), log"           ),
	AI( "3480", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f), log"           ),
	AI( "3481", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f), log"           ),
	AI( "3482", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f), log"           ),
	AI( "3483", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f), log"           ),
	AI( "3484", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f), log"           ),
	AI( "3485", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (f), log"           ),
	AI( "3490", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y), log"           ),
	AI( "3491", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y), log"           ),
	AI( "3492", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y), log"           ),
	AI( "3493", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y), log"           ),
	AI( "3494", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y), log"           ),
	AI( "3495", NO_FNC1, N,6,6,_, __, __, __, __,                     "HEIGHT (y), log"           ),
	AI( "3500", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2)"                ),
	AI( "3501", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2)"                ),
	AI( "3502", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2)"                ),
	AI( "3503", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2)"                ),
	AI( "3504", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2)"                ),
	AI( "3505", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2)"                ),
	AI( "3510", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2)"                ),
	AI( "3511", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2)"                ),
	AI( "3512", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2)"                ),
	AI( "3513", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2)"                ),
	AI( "3514", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2)"                ),
	AI( "3515", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2)"                ),
	AI( "3520", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2)"                ),
	AI( "3521", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2)"                ),
	AI( "3522", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2)"                ),
	AI( "3523", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2)"                ),
	AI( "3524", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2)"                ),
	AI( "3525", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2)"                ),
	AI( "3530", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2), log"           ),
	AI( "3531", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2), log"           ),
	AI( "3532", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2), log"           ),
	AI( "3533", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2), log"           ),
	AI( "3534", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2), log"           ),
	AI( "3535", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (i^2), log"           ),
	AI( "3540", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2), log"           ),
	AI( "3541", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2), log"           ),
	AI( "3542", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2), log"           ),
	AI( "3543", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2), log"           ),
	AI( "3544", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2), log"           ),
	AI( "3545", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (f^2), log"           ),
	AI( "3550", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2), log"           ),
	AI( "3551", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2), log"           ),
	AI( "3552", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2), log"           ),
	AI( "3553", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2), log"           ),
	AI( "3554", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2), log"           ),
	AI( "3555", NO_FNC1, N,6,6,_, __, __, __, __,                     "AREA (y^2), log"           ),
	AI( "3560", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (t)"            ),
	AI( "3561", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (t)"            ),
	AI( "3562", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (t)"            ),
	AI( "3563", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (t)"            ),
	AI( "3564", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (t)"            ),
	AI( "3565", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET WEIGHT (t)"            ),
	AI( "3570", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (oz)"           ),
	AI( "3571", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (oz)"           ),
	AI( "3572", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (oz)"           ),
	AI( "3573", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (oz)"           ),
	AI( "3574", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (oz)"           ),
	AI( "3575", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (oz)"           ),
	AI( "3600", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (q)"            ),
	AI( "3601", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (q)"            ),
	AI( "3602", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (q)"            ),
	AI( "3603", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (q)"            ),
	AI( "3604", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (q)"            ),
	AI( "3605", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (q)"            ),
	AI( "3610", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (g)"            ),
	AI( "3611", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (g)"            ),
	AI( "3612", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (g)"            ),
	AI( "3613", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (g)"            ),
	AI( "3614", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (g)"            ),
	AI( "3615", NO_FNC1, N,6,6,_, __, __, __, __,                     "NET VOLUME (g)"            ),
	AI( "3620", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (q), log"           ),
	AI( "3621", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (q), log"           ),
	AI( "3622", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (q), log"           ),
	AI( "3623", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (q), log"           ),
	AI( "3624", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (q), log"           ),
	AI( "3625", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (q), log"           ),
	AI( "3630", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (g), log"           ),
	AI( "3631", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (g), log"           ),
	AI( "3632", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (g), log"           ),
	AI( "3633", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (g), log"           ),
	AI( "3634", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (g), log"           ),
	AI( "3635", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (g), log"           ),
	AI( "3640", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3)"              ),
	AI( "3641", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3)"              ),
	AI( "3642", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3)"              ),
	AI( "3643", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3)"              ),
	AI( "3644", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3)"              ),
	AI( "3645", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3)"              ),
	AI( "3650", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3)"              ),
	AI( "3651", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3)"              ),
	AI( "3652", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3)"              ),
	AI( "3653", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3)"              ),
	AI( "3654", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3)"              ),
	AI( "3655", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3)"              ),
	AI( "3660", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3)"              ),
	AI( "3661", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3)"              ),
	AI( "3662", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3)"              ),
	AI( "3663", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3)"              ),
	AI( "3664", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3)"              ),
	AI( "3665", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3)"              ),
	AI( "3670", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3), log"         ),
	AI( "3671", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3), log"         ),
	AI( "3672", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3), log"         ),
	AI( "3673", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3), log"         ),
	AI( "3674", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3), log"         ),
	AI( "3675", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (i^3), log"         ),
	AI( "3680", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3), log"         ),
	AI( "3681", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3), log"         ),
	AI( "3682", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3), log"         ),
	AI( "3683", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3), log"         ),
	AI( "3684", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3), log"         ),
	AI( "3685", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (f^3), log"         ),
	AI( "3690", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3), log"         ),
	AI( "3691", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3), log"         ),
	AI( "3692", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3), log"         ),
	AI( "3693", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3), log"         ),
	AI( "3694", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3), log"         ),
	AI( "3695", NO_FNC1, N,6,6,_, __, __, __, __,                     "VOLUME (y^3), log"         ),
	AI( "37"  , FNC1   , N,1,8,_, __, __, __, __,                     "COUNT"                     ),
	AI( "3900", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3901", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3902", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3903", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3904", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3905", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3906", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3907", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3908", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3909", FNC1   , N,1,15,_, __, __, __, __,                    "AMOUNT"                    ),
	AI( "3910", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3911", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3912", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3913", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3914", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3915", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3916", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3917", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3918", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3919", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "AMOUNT"                    ),
	AI( "3920", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3921", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3922", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3923", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3924", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3925", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3926", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3927", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3928", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3929", FNC1   , N,1,15,_, __, __, __, __,                    "PRICE"                     ),
	AI( "3930", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3931", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3932", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3933", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3934", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3935", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3936", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3937", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3938", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3939", FNC1   , N,3,3,_, N,1,15,_, __, __, __,               "PRICE"                     ),
	AI( "3940", FNC1   , N,4,4,_, __, __, __, __,                     "PRCNT OFF"                 ),
	AI( "3941", FNC1   , N,4,4,_, __, __, __, __,                     "PRCNT OFF"                 ),
	AI( "3942", FNC1   , N,4,4,_, __, __, __, __,                     "PRCNT OFF"                 ),
	AI( "3943", FNC1   , N,4,4,_, __, __, __, __,                     "PRCNT OFF"                 ),
	AI( "3950", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE/UoM"                 ),
	AI( "3951", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE/UoM"                 ),
	AI( "3952", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE/UoM"                 ),
	AI( "3953", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE/UoM"                 ),
	AI( "3954", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE/UoM"                 ),
	AI( "3955", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE/UoM"                 ),
	AI( "400" , FNC1   , X,1,30,_, __, __, __, __,                    "ORDER NUMBER"              ),
	AI( "401" , FNC1   , X,1,30,_, __, __, __, __,                    "GINC"                      ),
	AI( "402" , FNC1   , N,17,17,csum, __, __, __, __,                "GSIN"                      ),
	AI( "403" , FNC1   , X,1,30,_, __, __, __, __,                    "ROUTE"                     ),
	AI( "410" , NO_FNC1, N,13,13,csum, __, __, __, __,                "SHIP TO LOC"               ),
	AI( "411" , NO_FNC1, N,13,13,csum, __, __, __, __,                "BILL TO"                   ),
	AI( "412" , NO_FNC1, N,13,13,csum, __, __, __, __,                "PURCHASE FROM"             ),
	AI( "413" , NO_FNC1, N,13,13,csum, __, __, __, __,                "SHIP FOR LOC"              ),
	AI( "414" , NO_FNC1, N,13,13,csum, __, __, __, __,                "LOC NO."                   ),
	AI( "415" , NO_FNC1, N,13,13,csum, __, __, __, __,                "PAY TO"                    ),
	AI( "416" , NO_FNC1, N,13,13,csum, __, __, __, __,                "PROD/SERV LOC"             ),
	AI( "417" , NO_FNC1, N,13,13,csum, __, __, __, __,                "PARTY"                     ),
	AI( "420" , FNC1   , X,1,20,_, __, __, __, __,                    "SHIP TO POST"              ),
	AI( "421" , FNC1   , N,3,3,_, X,1,9,_, __, __, __,                "SHIP TO POST"              ),
	AI( "422" , FNC1   , N,3,3,_, __, __, __, __,                     "ORIGIN"                    ),
	AI( "423" , FNC1   , N,3,15,_, __, __, __, __,                    "COUNTRY - INITIAL PROCESS" ),
	AI( "424" , FNC1   , N,3,3,_, __, __, __, __,                     "COUNTRY - PROCESS"         ),
	AI( "425" , FNC1   , N,3,15,_, __, __, __, __,                    "COUNTRY - DISASSEMBLY"     ),
	AI( "426" , FNC1   , N,3,3,_, __, __, __, __,                     "COUNTRY - FULL PROCESS"    ),
	AI( "427" , FNC1   , X,1,3,_, __, __, __, __,                     "ORIGIN SUBDIVISION"        ),
	AI( "4300", FNC1   , X,1,35,_, __, __, __, __,                    "SHIP TO COMP"              ),
	AI( "4301", FNC1   , X,1,35,_, __, __, __, __,                    "SHIP TO NAME"              ),
	AI( "4302", FNC1   , X,1,70,_, __, __, __, __,                    "SHIP TO ADD1"              ),
	AI( "4303", FNC1   , X,1,70,_, __, __, __, __,                    "SHIP TO ADD2"              ),
	AI( "4304", FNC1   , X,1,70,_, __, __, __, __,                    "SHIP TO SUB"               ),
	AI( "4305", FNC1   , X,1,70,_, __, __, __, __,                    "SHIP TO LOC"               ),
	AI( "4306", FNC1   , X,1,70,_, __, __, __, __,                    "SHIP TO REG"               ),
	AI( "4307", FNC1   , X,2,2,_, __, __, __, __,                     "SHIP TO COUNTRY"           ),
	AI( "4308", FNC1   , X,1,30,_, __, __, __, __,                    "SHIP TO PHONE"             ),
	AI( "4310", FNC1   , X,1,35,_, __, __, __, __,                    "RTN TO COMP"               ),
	AI( "4311", FNC1   , X,1,35,_, __, __, __, __,                    "RTN TO NAME"               ),
	AI( "4312", FNC1   , X,1,70,_, __, __, __, __,                    "RTN TO ADD1"               ),
	AI( "4313", FNC1   , X,1,70,_, __, __, __, __,                    "RTN TO ADD2"               ),
	AI( "4314", FNC1   , X,1,70,_, __, __, __, __,                    "RTN TO SUB"                ),
	AI( "4315", FNC1   , X,1,70,_, __, __, __, __,                    "RTN TO LOC"                ),
	AI( "4316", FNC1   , X,1,70,_, __, __, __, __,                    "RTN TO REG"                ),
	AI( "4317", FNC1   , X,2,2,_, __, __, __, __,                     "RTN TO COUNTRY"            ),
	AI( "4318", FNC1   , X,1,20,_, __, __, __, __,                    "RTN TO POST"               ),
	AI( "4319", FNC1   , X,1,30,_, __, __, __, __,                    "RTN TO PHONE"              ),
	AI( "4320", FNC1   , X,1,35,_, __, __, __, __,                    "SRV DESCRIPTION"           ),
	AI( "4321", FNC1   , N,1,1,_, __, __, __, __,                     "DANGEROUS GOODS"           ),
	AI( "4322", FNC1   , N,1,1,_, __, __, __, __,                     "AUTH LEAVE"                ),
	AI( "4323", FNC1   , N,1,1,_, __, __, __, __,                     "SIG REQUIRED"              ),
	AI( "4324", FNC1   , N,6,6,_, N,4,4,_, __, __, __,                "NBEF DEL DT."              ),
	AI( "4325", FNC1   , N,6,6,_, N,4,4,_, __, __, __,                "NAFT DEL DT."              ),
	AI( "4326", FNC1   , N,6,6,_, __, __, __, __,                     "REL DATE"                  ),
	AI( "7001", FNC1   , N,13,13,_, __, __, __, __,                   "NSN"                       ),
	AI( "7002", FNC1   , X,1,30,_, __, __, __, __,                    "MEAT CUT"                  ),
	AI( "7003", FNC1   , N,6,6,_, N,4,4,_, __, __, __,                "EXPIRY TIME"               ),
	AI( "7004", FNC1   , N,1,4,_, __, __, __, __,                     "ACTIVE POTENCY"            ),
	AI( "7005", FNC1   , X,1,12,_, __, __, __, __,                    "CATCH AREA"                ),
	AI( "7006", FNC1   , N,6,6,_, __, __, __, __,                     "FIRST FREEZE DATE"         ),
	AI( "7007", FNC1   , N,6,6,_, N,0,6,_, __, __, __,                "HARVEST DATE"              ),
	AI( "7008", FNC1   , X,1,3,_, __, __, __, __,                     "AQUATIC SPECIES"           ),
	AI( "7009", FNC1   , X,1,10,_, __, __, __, __,                    "FISHING GEAR TYPE"         ),
	AI( "7010", FNC1   , X,1,2,_, __, __, __, __,                     "PROD METHOD"               ),
	AI( "7020", FNC1   , X,1,20,_, __, __, __, __,                    "REFURB LOT"                ),
	AI( "7021", FNC1   , X,1,20,_, __, __, __, __,                    "FUNC STAT"                 ),
	AI( "7022", FNC1   , X,1,20,_, __, __, __, __,                    "REV STAT"                  ),
	AI( "7023", FNC1   , X,1,30,_, __, __, __, __,                    "GIAI - ASSEMBLY"           ),
	AI( "7030", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7031", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7032", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7033", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7034", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7035", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7036", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7037", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7038", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7039", FNC1   , N,3,3,_, X,1,27,_, __, __, __,               "PROCESSOR # s"             ),
	AI( "7040", FNC1   , N,1,1,_, X,1,1,_, X,1,1,_, X,1,1,_, __,      "UIC+EXT"                   ),
	AI( "710" , FNC1   , X,1,20,_, __, __, __, __,                    "NHRN PZN"                  ),
	AI( "711" , FNC1   , X,1,20,_, __, __, __, __,                    "NHRN CIP"                  ),
	AI( "712" , FNC1   , X,1,20,_, __, __, __, __,                    "NHRN CN"                   ),
	AI( "713" , FNC1   , X,1,20,_, __, __, __, __,                    "NHRN DRN"                  ),
	AI( "714" , FNC1   , X,1,20,_, __, __, __, __,                    "NHRN AIM"                  ),
	AI( "7230", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7231", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7232", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7233", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7234", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7235", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7236", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7237", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7238", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7239", FNC1   , X,2,2,_, X,1,28,_, __, __, __,               "CERT # s"                  ),
	AI( "7240", FNC1   , X,1,20,_, __, __, __, __,                    "PROTOCOL"                  ),
	AI( "8001", FNC1   , N,4,4,_, N,5,5,_, N,3,3,_, N,1,1,_, N,1,1,_, "DIMENSIONS"                ),
	AI( "8002", FNC1   , X,1,20,_, __, __, __, __,                    "CMT NO."                   ),
	AI( "8003", FNC1   , N,1,1,_, N,13,13,csum, X,0,16,_, __, __,     "GRAI"                      ),
	AI( "8004", FNC1   , X,1,30,_, __, __, __, __,                    "GIAI"                      ),
	AI( "8005", FNC1   , N,6,6,_, __, __, __, __,                     "PRICE PER UNIT"            ),
	AI( "8006", FNC1   , N,14,14,csum, N,4,4,_, __, __, __,           "ITIP"                      ),
	AI( "8007", FNC1   , X,1,34,_, __, __, __, __,                    "IBAN"                      ),
	AI( "8008", FNC1   , N,8,8,_, N,0,4,_, __, __, __,                "PROD TIME"                 ),
	AI( "8009", FNC1   , X,1,50,_, __, __, __, __,                    "OPTSEN"                    ),
	AI( "8010", FNC1   , C,1,30,_, __, __, __, __,                    "CPID"                      ),
	AI( "8011", FNC1   , N,1,12,_, __, __, __, __,                    "CPID SERIAL"               ),
	AI( "8012", FNC1   , X,1,20,_, __, __, __, __,                    "VERSION"                   ),
	AI( "8013", FNC1   , X,1,25,_, __, __, __, __,                    "GMN"                       ),
	AI( "8017", FNC1   , N,18,18,csum, __, __, __, __,                "GSRN - PROVIDER"           ),
	AI( "8018", FNC1   , N,18,18,csum, __, __, __, __,                "GSRN - RECIPIENT"          ),
	AI( "8019", FNC1   , N,1,10,_, __, __, __, __,                    "SRIN"                      ),
	AI( "8020", FNC1   , X,1,25,_, __, __, __, __,                    "REF NO."                   ),
	AI( "8026", FNC1   , N,14,14,csum, N,4,4,_, __, __, __,           "ITIP CONTENT"              ),
	AI( "8110", FNC1   , X,1,70,_, __, __, __, __,                    ""                          ),
	AI( "8111", FNC1   , N,4,4,_, __, __, __, __,                     "POINTS"                    ),
	AI( "8112", FNC1   , X,1,70,_, __, __, __, __,                    ""                          ),
	AI( "8200", FNC1   , X,1,70,_, __, __, __, __,                    "PRODUCT URL"               ),
	AI( "90"  , FNC1   , X,1,30,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "91"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "92"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "93"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "94"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "95"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "96"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "97"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "98"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
	AI( "99"  , FNC1   , X,1,90,_, __, __, __, __,                    "INTERNAL"                  ),
};


static const struct aiEntry* lookup_ai_entry(char *p, bool prefix) {

	int ai_table_len = sizeof(ai_table) / sizeof(ai_table[0]);
	int i;
	const struct aiEntry *entry;
	size_t ailen;

	// Walk the AI table to find a (prefix) match
	for (i = 0; i < ai_table_len; i++) {
		entry = &ai_table[i];
		ailen = strlen(entry->ai);
		if (strlen(p) < ailen)
			continue;
		if (prefix &&		// Lookup whether an AI is a prefix of a given message
		    strncmp(p, entry->ai, ailen) == 0)
			break;
		else if (strcmp(p, entry->ai) == 0)			// Given the entire AI
			break;
	}

	if (i == ai_table_len)		// Not found
		return NULL;

	return entry;

}


/*
 *  Validate string between start and end pointers according to rules for an AI
 *
 */
static size_t validate_ai_val(gs1_encoder *ctx, const struct aiEntry *entry, char *start, char *end) {

	const struct aiComponent *part;
	int parts_len = sizeof(ai_table[0].parts) / sizeof(ai_table[0].parts[0]);
	int linters_len = sizeof(ai_table[0].parts[0].linters) / sizeof(ai_table[0].parts[0].linters[0]);
	int i, j;
	char compval[91];		// Maximum AI value length is 90
	size_t complen;
	linter_t linter;
	char *p, *r;

	assert(ctx);
	assert(entry);
	assert(start);
	assert(end);
	assert(end >= start);

	DEBUG_PRINT("  Considering AI (%s): %s (first %d characters)\n", entry->ai, start, (int)(end-start));

	p = start;
	r = end;
	if (p == r) {
		sprintf(ctx->errMsg, "AI (%s) data is empty", entry->ai);
		ctx->errFlag = true;
		return 0;
	}

	for (i = 0; i < parts_len; i++) {
		part = &entry->parts[i];
		if (part->cset == cset_none)
			break;

		complen = (size_t)(r-p);	// Until given FNC1 or end...
		if (part->max < r-p)
			complen = part->max;	// ... reduced to max length of component
		strncpy(compval, p, complen);
		compval[complen] = '\0';
		p += complen;

		DEBUG_PRINT("    Validating component: %s\n", compval);

		if (complen < part->min) {
			sprintf(ctx->errMsg, "AI (%s) data is too short", entry->ai);
			ctx->errFlag = true;
			return 0;
		}

		// Run the cset linter
		linter = part->cset == cset_N ? lint_csetNumeric : lint_cset82;
		if (!linter(ctx, entry, compval))
			return 0;

		// Run each additional linter on the component
		for (j = 0; j < linters_len; j++) {
			if (!part->linters[j])
				break;
			if (!part->linters[j](ctx, entry, compval))
				return 0;
		}
	}

	return (size_t)(p-start);	// Amount of data that validation consumed

}


// Write to dataStr checking for overflow
#define writeDataStr(v) do {						\
	if (strlen(dataStr) + strlen(v) > MAX_DATA)			\
		goto fail;						\
	strcat(dataStr, v);						\
} while (0)

#define nwriteDataStr(v,l) do {						\
	if (strlen(dataStr) + l > MAX_DATA)				\
		goto fail;						\
	strncat(dataStr, v, l);						\
} while (0)


// Convert GS1 AI syntax data to regular data string with # = FNC1
bool gs1_parseAIdata(gs1_encoder *ctx, char *aiData, char *dataStr) {

	char *p;
	char *r, *outval;
	int i;
	size_t minlen, maxlen;
	bool fnc1req = true;
	const struct aiEntry *entry;
	int parts_len = sizeof(ai_table[0].parts) / sizeof(ai_table[0].parts[0]);

	assert(ctx);
	assert(aiData);

	*dataStr = '\0';
	*ctx->errMsg = '\0';
	ctx->errFlag = false;

	DEBUG_PRINT("\nParsing AI data: %s\n", aiData);

	p = aiData;
	while (*p) {

		if (*p++ != '(') goto fail; 			// Expect start of AI
		if (!(r = strchr(p, ')'))) goto fail;		// Find end of A

		*r = '\0';					// Delimit the end of the AI pointed to by p
		entry = lookup_ai_entry(p, false);
		*r++ = ')';					// Put back the original ")"
		if (entry == NULL) {
			sprintf(ctx->errMsg, "Unrecognised AI: %.4s", p);
			goto fail;
		}

		if (fnc1req)
			writeDataStr("#");			// Write FNC1, if required
		writeDataStr(entry->ai);			// Write AI

		fnc1req = true;					// Determine whether FNC1 required before next AI
		for (i = 0; i < (int)(sizeof(fixedAIprefixes) / sizeof(fixedAIprefixes[0])); i++)
			if (strncmp(fixedAIprefixes[i], entry->ai, 2) == 0)
				fnc1req = false;

		if (!*r) goto fail;				// Fail if message ends after AI and no value

		outval = dataStr + strlen(dataStr);		// Record the current start of the output value

again:

		if ((p = strchr(r, '(')) == NULL)
			p = r + strlen(r);			// Move the pointer to the end if no more AIs

		if (*p != '\0' && *(p-1) == '\\') {		// This bracket is an escaped data character
			nwriteDataStr(r, (size_t)(p-r-1));	// Write up to the escape character
			writeDataStr("(");			// Write the data bracket
			r = p+1;				// And keep going
			goto again;
		}

		nwriteDataStr(r, (size_t)(p-r));		// Write the remainder of the value

		// Do an overall AI length check prior to performing
		// component-based validation since reporting issues such as
		// checksum failure isn't helpful when the AI is too long
		minlen = 0;
		maxlen = 0;
		for (i = 0; i < parts_len; i++) {
			minlen += entry->parts[i].min;
			maxlen += entry->parts[i].max;
		}
		if (strlen(outval) < minlen) {
			sprintf(ctx->errMsg, "AI (%s) value is too short", entry->ai);
			goto fail;
		}
		if (strlen(outval) > maxlen) {
			sprintf(ctx->errMsg, "AI (%s) value is too long", entry->ai);
			goto fail;
		}

		// Also forbid data "#" characters at this stage so we don't conflate with FNC1
		if (strchr(outval, '#') != NULL) {
			sprintf(ctx->errMsg, "AI (%s) contains illegal # character", entry->ai);
			goto fail;
		}

	}

	DEBUG_PRINT("Parsing AI data successful: %s\n", dataStr);

	// Now validate the data that we have written
	return gs1_processAIdata(ctx, dataStr);

fail:

	if (*ctx->errMsg == '\0')
		strcpy(ctx->errMsg, "Failed to parse AI data");
	ctx->errFlag = true;

	DEBUG_PRINT("Parsing AI data failed: %s\n", ctx->errMsg);

	*dataStr = '\0';
	return false;

}


bool gs1_processAIdata(gs1_encoder *ctx, char *dataStr) {

	char *p, *r;
	size_t vallen;
	const struct aiEntry *entry;

	assert(ctx);
	assert(dataStr);

	*ctx->errMsg = '\0';
	ctx->errFlag = false;

	p = dataStr;

	// Ensure FNC1 in first
	if (!*p || *p++ != '#') {
		strcpy(ctx->errMsg, "Missing FNC1 in first position");
		ctx->errFlag = true;
		return false;
	}

	// Must have some AI data
	if (!*p) {
		strcpy(ctx->errMsg, "The AI data is empty");
		ctx->errFlag = true;
		return false;
	}

	while (*p) {

		if ((entry = lookup_ai_entry(p, true)) == NULL) {
			sprintf(ctx->errMsg, "Unrecognised AI: %.4s", p);
			ctx->errFlag = true;
			return false;
		}

		p += strlen(entry->ai);

		// r points to the next FNC1 or end of string...
		if ((r = strchr(p, '#')) == NULL)
			r = p + strlen(p);

		// Validate and return how much was consumed
		if ((vallen = validate_ai_val(ctx, entry, p, r)) == 0)
			return false;

		// Add to the aiData
		if (ctx->numAIs < MAX_AIS) {
			ctx->aiData[ctx->numAIs].aiEntry = entry;
			ctx->aiData[ctx->numAIs].value = p;
			ctx->aiData[ctx->numAIs].vallen = (uint8_t)vallen;
			ctx->numAIs++;
		} else {
			strcpy(ctx->errMsg, "Too many AIs");
			ctx->errFlag = true;
			return false;
		}

		// After AIs requiring FNC1, we expect to find an FNC1 or be at the end
		p += vallen;
		if (entry->fnc1 && *p != '#' && *p != '\0') {
			sprintf(ctx->errMsg, "AI (%s) data is too long", entry->ai);
			ctx->errFlag = true;
			return false;
		}

		// Skip FNC1, even at end of fixed-length AIs
		if (*p == '#')
			p++;

	}

	return true;

}


// Validate and set the parity digit
bool gs1_validateParity(uint8_t *str) {

	int weight;
	int parity = 0;

	assert(*str);

	weight = strlen((char*)str) % 2 == 0 ? 3 : 1;
	while (*(str+1)) {
		parity += weight * (*str++ - '0');
		weight = 4 - weight;
	}
	parity = (10 - parity%10) % 10;

	if (parity + '0' == *str) return true;

	*str = (uint8_t)(parity + '0');		// Recalculate
	return false;

}


bool gs1_allDigits(uint8_t *str) {

	assert(str);

	while (*str) {
		if (*str < '0' || *str >'9')
			return false;
		str++;
	}
	return true;

}



#ifdef UNIT_TESTS

#define TEST_NO_MAIN
#include "acutest.h"


static void test_parseAIdata(gs1_encoder *ctx, bool should_succeed, char *aiData, char* expect) {

	char in[256];
	char casename[256];

	sprintf(casename, "%s => %s", aiData, expect);
	TEST_CASE(casename);

	strcpy(in, aiData);
	TEST_CHECK(gs1_parseAIdata(ctx, in, ctx->dataStr) ^ !should_succeed);
	if (should_succeed)
		TEST_CHECK(strcmp(ctx->dataStr, expect) == 0);
	TEST_MSG("Given: %s; Got: %s; Expected: %s; Err: %s", aiData, ctx->dataStr, expect, ctx->errMsg);

}


/*
 *  Convert a human-readable AI string to plain format
 *
 */
void test_gs1_parseAIdata(void) {

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	test_parseAIdata(ctx, true,  "(01)12345678901231", "#0112345678901231");
	test_parseAIdata(ctx, true,  "(10)12345", "#1012345");
	test_parseAIdata(ctx, true,  "(01)12345678901231(10)12345", "#01123456789012311012345");	// No FNC1 after (01)
	test_parseAIdata(ctx, true,  "(3100)123456(10)12345", "#31001234561012345");			// No FNC1 after (3100)
	test_parseAIdata(ctx, true,  "(10)12345(11)991225", "#1012345#11991225");			// FNC1 after (10)
	test_parseAIdata(ctx, true,  "(3900)12345(11)991225", "#390012345#11991225");			// FNC1 after (3900)
	test_parseAIdata(ctx, true,  "(10)12345\\(11)991225", "#1012345(11)991225");			// Escaped bracket
	test_parseAIdata(ctx, true,  "(10)12345\\(", "#1012345(");					// At end if fine

	test_parseAIdata(ctx, false, "(10)(11)98765", "");						// Value must not be empty
	test_parseAIdata(ctx, false, "(10)12345(11)", "");						// Value must not be empty
	test_parseAIdata(ctx, false, "(1A)12345", "");							// AI must be numeric
	test_parseAIdata(ctx, false, "1(12345", "");							// Must start with AI
	test_parseAIdata(ctx, false, "12345", "");							// Must start with AI
	test_parseAIdata(ctx, false, "()12345", "");							// AI too short
	test_parseAIdata(ctx, false, "(1)12345", "");							// AI too short
	test_parseAIdata(ctx, false, "(12345)12345", "");						// AI too long
	test_parseAIdata(ctx, false, "(15", "");							// AI must terminate
	test_parseAIdata(ctx, false, "(1", "");								// AI must terminate
	test_parseAIdata(ctx, false, "(", "");								// AI must terminate
	test_parseAIdata(ctx, false, "(01)123456789012312(10)12345", "");				// Fixed-length AI too long
	test_parseAIdata(ctx, false, "(10)12345#", "");							// Reject "#": Conflated with FNC1
	test_parseAIdata(ctx, false, "(17)9(90)217", "");						// Should not parse to #17990217

	gs1_encoder_free(ctx);

}


static void test_processAIdata(gs1_encoder *ctx, bool should_succeed, char *dataStr) {

	char casename[256];

	sprintf(casename, "%s", dataStr);
	TEST_CASE(casename);

	strcpy(ctx->dataStr, dataStr);
	TEST_CHECK(gs1_processAIdata(ctx, ctx->dataStr) ^ !should_succeed);
	TEST_MSG(ctx->errMsg);

}


void test_gs1_processAIdata(void) {

	gs1_encoder* ctx = gs1_encoder_init(NULL);

	test_processAIdata(ctx, false, "");						// No FNC1 in first position
	test_processAIdata(ctx, false, "991234");					// No FNC1 in first position
	test_processAIdata(ctx, false, "#");						// FNC1 in first but no AIs
	test_processAIdata(ctx, false, "#891234");					// No such AI

	test_processAIdata(ctx, true,  "#991234");

	test_processAIdata(ctx, false, "#99~ABC");					// Bad CSET82 character
 	test_processAIdata(ctx, false, "#99ABC~");					// Bad CSET82 character

	test_processAIdata(ctx, true,  "#0112345678901231");				// N14, no FNC1 required
	test_processAIdata(ctx, false, "#01A2345678901231");				// Bad numeric character
	test_processAIdata(ctx, false, "#011234567890123A");				// Bad numeric character
	test_processAIdata(ctx, false, "#0112345678901234");				// Incorrect check digit (csum linter)
	test_processAIdata(ctx, false, "#011234567890123");				// Too short
	test_processAIdata(ctx, false, "#01123456789012312");				// No such AI (2). Can't be "too long" since FNC1 not required

	test_processAIdata(ctx, true,  "#0112345678901231#");				// Tolerate superflous FNC1
	test_processAIdata(ctx, false, "#011234567890123#");				// Short, with superflous FNC1
	test_processAIdata(ctx, false, "#01123456789012345#");				// Long, with superflous FNC1 (no following AIs)
	test_processAIdata(ctx, false, "#01123456789012345#991234");			// Long, with superflous FNC1 and meaningless AI (5#..)

	test_processAIdata(ctx, true,  "#0112345678901231991234");			// Fixed-length, run into next AI (01)...(99)...
	test_processAIdata(ctx, true,  "#0112345678901231#991234");			// Tolerate superflous FNC1

	test_processAIdata(ctx, true,  "#2421");					// N1..6; FNC1 required
	test_processAIdata(ctx, true,  "#24212");
	test_processAIdata(ctx, true,  "#242123");
	test_processAIdata(ctx, true,  "#2421234");
	test_processAIdata(ctx, true,  "#24212345");
	test_processAIdata(ctx, true,  "#242123456");
	test_processAIdata(ctx, true,  "#242123456#10ABC123");				// Limit, then following AI
	test_processAIdata(ctx, true,  "#242123456#");					// Tolerant of FNC1 at end of data
	test_processAIdata(ctx, false, "#2421234567");					// Data too long

	test_processAIdata(ctx, true,  "#81111234");					// N4; FNC1 required
	test_processAIdata(ctx, false, "#8111123");					// Too short
	test_processAIdata(ctx, false, "#811112345");					// Too long
	test_processAIdata(ctx, true,  "#81111234#10ABC123");				// Followed by another AI

	test_processAIdata(ctx, true,  "#800112341234512398");				// N4-5-3-1-1; FNC1 required
	test_processAIdata(ctx, false, "#80011234123451239");				// Too short
	test_processAIdata(ctx, false, "#8001123412345123981");				// Too long
	test_processAIdata(ctx, true,  "#800112341234512398#0112345678901231");
	test_processAIdata(ctx, false, "#80011234123451239#0112345678901231");		// Too short
	test_processAIdata(ctx, false, "#8001123412345123981#01123456789012312");	// Too long

	test_processAIdata(ctx, true,  "#800302112345678900ABC");			// N1 N13,csum X0..16; FNC1 required
	test_processAIdata(ctx, false, "#800302112345678901ABC");			// Bad check digit on N13 component
	test_processAIdata(ctx, true,  "#800302112345678900");				// Empty final component
	test_processAIdata(ctx, true,  "#800302112345678900#10ABC123");			// Empty final component and following AI
	test_processAIdata(ctx, true,  "#800302112345678900ABCDEFGHIJKLMNOP");		// Empty final component and following AI
	test_processAIdata(ctx, false, "#800302112345678900ABCDEFGHIJKLMNOPQ");		// Empty final component and following AI

	test_processAIdata(ctx, true,  "#7230121234567890123456789012345678");		// X2 X1..28; FNC1 required
	test_processAIdata(ctx, false, "#72301212345678901234567890123456789");		// Too long
	test_processAIdata(ctx, true,  "#7230123");					// Shortest
	test_processAIdata(ctx, false, "#723012");					// Too short

	gs1_encoder_free(ctx);

}


void test_gs1_validateParity(void) {

	char good_gtin14[] = "24012345678905";
	char bad_gtin14[]  = "24012345678909";
	char good_gtin13[] = "2112233789657";
	char bad_gtin13[]  = "2112233789658";
	char good_gtin12[] = "416000336108";
	char bad_gtin12[]  = "416000336107";
	char good_gtin8[]  = "02345680";
	char bad_gtin8[]   = "02345689";

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin14));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin14));
	TEST_CHECK(bad_gtin14[13] == '5');		// Recomputed

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin13));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin13));
	TEST_CHECK(bad_gtin13[12] == '7');		// Recomputed

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin12));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin12));
	TEST_CHECK(bad_gtin12[11] == '8');		// Recomputed

	TEST_CHECK(gs1_validateParity((uint8_t*)good_gtin8));
	TEST_CHECK(!gs1_validateParity((uint8_t*)bad_gtin8));
	TEST_CHECK(bad_gtin8[7] == '0');		// Recomputed

}


#endif  /* UNIT_TESTS */

