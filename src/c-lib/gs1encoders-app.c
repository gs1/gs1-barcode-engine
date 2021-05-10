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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gs1encoders.h"

#define RELEASE __DATE__

// Some limits for user input
#define MAX_PIXMULT 12
#define MAX_KEYDATA 120
#define MAX_LINHT 500 // max UCC/EAN-128 height in X


static const char* SYMBOLOGY_NAMES[] =
{
	"",  // Spacer
	"GS1 DataBar",
	"GS1 DataBar Truncated",
	"GS1 DataBar Stacked",
	"GS1 DataBar Stacked Omnidirectional",
	"GS1 DataBar Limited",
	"GS1 DataBar Expanded (Stacked)",
	"UPC-A",
	"UPC-E",
	"EAN-13",
	"EAN-8",
	"GS1-128 with CC-A or CC-B",
	"GS1-128 with CC-C"
};

// Replacement for the deprecated gets(3) function
static char* gets(char* in) {

	char* s;

	s = fgets(in,MAX_KEYDATA+1,stdin);
	if (s != NULL) {
		s[strcspn(s, "\r\n")] = 0;
	}
	return s;
}

static bool getSym(gs1_encoder *ctx) {

	char inpStr[MAX_KEYDATA+1];
	int i;

	while (true) {
		printf("\nGS1 Encoders (Built " RELEASE "):");
		printf("\n\nCopyright (c) 2020 GS1 AISBL. License: Apache-2.0");
		printf("\n\nMAIN MENU:");
		printf("\n 0)  Exit Program");
		for (i = 1; i < sNUMSYMS; i += 2) {
			printf("\n%2d)  %-25s     %2d)  %-25s",
				i, SYMBOLOGY_NAMES[i], i + 1, SYMBOLOGY_NAMES[i + 1]);
		}
		printf("\n\nEnter symbology type or 0 to exit: ");
		if (gets(inpStr) == NULL) {
			printf("PLEASE ENTER 0 THROUGH 12.");
			continue;
		}
		gs1_encoder_setSym(ctx, atoi(inpStr));
		if (gs1_encoder_getSym(ctx) == 0) {
			return(false);
		}
		if (gs1_encoder_getSym(ctx) > 12) {
			printf("PLEASE ENTER 0 THROUGH 12.");
			continue;
		}
		return(true);
	}
}

static bool userInt(gs1_encoder *ctx) {

	int inMenu = true;
	int retFlag = true; // return is false if exit program
	char inpStr[MAX_KEYDATA+1];
	int menuVal, i;

	while (inMenu) {
		if (gs1_encoder_getSym(ctx) == sNONE) {
			if (!getSym(ctx)) {
				printf("DONE.\n");
				return(false);
			}
		}
		printf("\n\nData input string or file format:");
		switch (gs1_encoder_getSym(ctx)) {
			case sRSS14:
			case sRSS14T:
			case sRSS14S:
			case sRSS14SO:
			case sRSSLIM:
				printf("\n Primary data is up to 13 digits. Check digit must be omitted.");
				printf("\n 2D component data starts with 1st AI. Only interior FNC1s are needed.");
				break;
			case sRSSEXP:
				printf("\n GS1 DataBar Expanded (& 2D component) data starts with 1st AI. Only interior FNC1s are needed.");
				printf("\nSpecial data input characters:");
				break;
			case sUPCA:
				printf("\n Primary data is up to 11 digits. Check digit must be omitted.");
				printf("\n 2D component data starts with 1st AI. Only interior FNC1s are necessary.");
				break;
			case sUPCE:
				printf("\n Primary data (not zero suppressed) is up to 10 digits. Check digit must be omitted.");
				printf("\n 2D component data starts with 1st AI. Only interior FNC1s are necessary.");
				break;
			case sEAN13:
				printf("\n Primary data is up to 12 digits. Check digit must be omitted.");
				printf("\n 2D component data starts with 1st AI. Only interior FNC1s are necessary.");
				break;
			case sEAN8:
				printf("\n Primary data is up to 7 digits. Check digit must be omitted.");
				printf("\n 2D component data starts with 1st AI. Only interior FNC1s are necessary.");
				break;
			case sUCC128_CCA:
			case sUCC128_CCC:
				printf("\n Code 128 data starts with 1st AI. Only interior FNC1s are necessary.");
				printf("\n 2D component data starts with 1st AI. Only interior FNC1s are necessary.");
				break;
			default:
				printf("\nSYMBOL TYPE ERROR.");
				return(false);
		}
		printf("\n Special characters:");
		printf("\n   # (pound sign):   FNC1");
		printf("\n   | (vertical bar): Separates primary and secondary data");
		printf("\n   ^ (caret):        Symbol separator used to flag ]e1n format in 2D data");
		printf("\n\nMENU (Symbology: %s):", SYMBOLOGY_NAMES[gs1_encoder_getSym(ctx)]);
		printf("\n 0) Enter pixels per X. Current value = %d", gs1_encoder_getPixMult(ctx));
		printf("\n 1) Enter X pixels to undercut. Current value = %d", gs1_encoder_getXundercut(ctx));
		printf("\n 2) Enter Y pixels to undercut. Current value = %d", gs1_encoder_getYundercut(ctx));
		printf("\n 3) Enter %s output file name. Current name = %s",
							 (gs1_encoder_getBmp(ctx)) ? "BMP":"TIF", gs1_encoder_getOutFile(ctx));
		printf("\n 4) Select keyboard or file input source. Current = %s",
							 (gs1_encoder_getInputFlag(ctx) == 0) ? "keyboard":"file");
		if (gs1_encoder_getInputFlag(ctx) == 0) { // for kbd input
			printf("\n 5) Key enter data input string. %s output file will be created.",
							 (gs1_encoder_getBmp(ctx)) ? "BMP":"TIF");
		}
		else {
			printf("\n 5) Enter data input file name. %s output file will be created.",
							 (gs1_encoder_getBmp(ctx)) ? "BMP":"TIF");
		}
		printf("\n 6) Select TIF or BMP format. Current = %s",
							 (gs1_encoder_getBmp(ctx)) ? "BMP":"TIF");
		if (gs1_encoder_getSym(ctx) == sRSSEXP) {
			printf("\n 7) Select maximum segments per row. Current value = %d", gs1_encoder_getSegWidth(ctx));
		}
		if ((gs1_encoder_getSym(ctx) == sUCC128_CCA) || (gs1_encoder_getSym(ctx) == sUCC128_CCC)) {
			printf("\n 7) Enter GS1-128 height in X. Current value = %d",
								gs1_encoder_getLinHeight(ctx));
		}
		printf("\n 8) Enter separator row height. Current value = %d", gs1_encoder_getSepHt(ctx));
		printf("\n 9) Select another symbology or exit program");
		printf("\n\nMenu selection: ");
		if (gets(inpStr) == NULL) {
			printf("UNKNOWN OPTION. PLEASE ENTER 1 THROUGH 9.");
			continue;
		}
		menuVal = atoi(inpStr);
		switch (menuVal) {
			case 0:
				printf("\nEnter pixels per X. 1-%d valid: ",MAX_PIXMULT);
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER 1 THROUGH %d.",MAX_PIXMULT);
					continue;
				}
				i = atoi(inpStr);
				if (i < 1 || i > MAX_PIXMULT) {
					printf("OUT OF RANGE. PLEASE ENTER 1 THROUGH %d.",MAX_PIXMULT);
					continue;
				}
				gs1_encoder_setPixMult(ctx,i);
				if (i <= gs1_encoder_getXundercut(ctx)) {
					printf("RESETTING X UNDERCUT TO 0.");
					gs1_encoder_setXundercut(ctx, 0);
				}
				if (i <= gs1_encoder_getYundercut(ctx)) {
					printf("RESETTING Y UNDERCUT TO 0.");
					gs1_encoder_setYundercut(ctx, 0);
				}
				if (i*2 < gs1_encoder_getSepHt(ctx) || i > gs1_encoder_getSepHt(ctx)) {
					printf("RESETTING SEPRATOR HEIGHT TO %d.", i);
					gs1_encoder_setSepHt(ctx, i);
				}
				break;
			case 1:
				if (gs1_encoder_getPixMult(ctx) > 1) {
					printf("\nEnter X pixels to undercut. 0 through %d valid: ",
										gs1_encoder_getPixMult(ctx)-1);
					if (gets(inpStr) == NULL) {
						printf("UNKNOWN. PLEASE ENTER 0 THROUGH %d.",
										gs1_encoder_getPixMult(ctx)-1);
						continue;
					}
					i = atoi(inpStr);
					if (i < 0 || i > gs1_encoder_getPixMult(ctx)-1) {
						printf("OUT OF RANGE. PLEASE ENTER 0 THROUGH %d",
										gs1_encoder_getPixMult(ctx)-1);
						continue;
					}
					gs1_encoder_setXundercut(ctx, i);
				}
				else {
					printf("NO UNDERCUT WHEN 1 PIXEL PER X");
				}
				break;
			case 2:
				if (gs1_encoder_getPixMult(ctx) > 1) {
					printf("\nEnter Y pixels to undercut. 0 through %d valid: ",
										gs1_encoder_getPixMult(ctx)-1);
					if (gets(inpStr) == NULL) {
						printf("UNKNOWN. PLEASE ENTER 0 THROUGH %d.",
										gs1_encoder_getPixMult(ctx)-1);
						continue;
					}
					i = atoi(inpStr);
					if (i < 0 || i > gs1_encoder_getPixMult(ctx)-1) {
						printf("OUT OF RANGE. PLEASE ENTER 0 THROUGH %d",
										gs1_encoder_getPixMult(ctx)-1);
						continue;
					}
					gs1_encoder_setYundercut(ctx, i);
				}
				else {
					printf("NO UNDERCUT WHEN 1 PIXEL PER X");
				}
				break;
			case 3:
				printf("\nEnter %s output file name with extension: ",
							 (gs1_encoder_getBmp(ctx)) ? "BMP":"TIF");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN ENTRY.");
					continue;
				}
				if (strlen(inpStr) > GS1_ENCODERS_MAX_FNAME) {
					printf("NOT ACCEPTED. MUST BE 25 CHARACTERS OR FEWER.");
					continue;
				}
				gs1_encoder_setOutFile(ctx, inpStr);
				break;
			case 4:
				printf("\nEnter 0 for keyboard or 1 for file input: ");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER 0 or 1.");
					continue;
				}
				i = atoi(inpStr);
				if (!(i == 0 || i == 1)) {
					printf("OUT OF RANGE. PLEASE ENTER 0 or 1");
					continue;
				}
				gs1_encoder_setInputFlag(ctx, i);
				break;
			case 5:
			 if (gs1_encoder_getInputFlag(ctx) == 0) { // for kbd input
				printf("\nEnter linear|2d data. No more than %d characters: ", MAX_KEYDATA);
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN ENTRY.");
					continue;
				}
				gs1_encoder_setDataStr(ctx, inpStr);
				inMenu = false;
			 }
			 else {
				printf("\nEnter data input file name: ");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN ENTRY.");
					continue;
				}
				if (strlen(inpStr) > GS1_ENCODERS_MAX_FNAME) {
					printf("NOT ACCEPTED. MUST BE 25 CHARACTERS OR FEWER.");
					continue;
				}
				gs1_encoder_setDataFile(ctx, inpStr);
				inMenu = false;
			 }
			 break;
			case 6:
				printf("\nEnter 0 for TIF or 1 for BMP output: ");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER 0 or 1.");
					continue;
				}
				i = atoi(inpStr);
				if (!(i == 0 || i == 1)) {
					printf("OUT OF RANGE. PLEASE ENTER 0 or 1");
					continue;
				}
				if (gs1_encoder_getBmp(ctx) != i) {
					if (i == 0) {
						gs1_encoder_setOutFile(ctx, "out.tif");
					}
					else {
						gs1_encoder_setOutFile(ctx, "out.bmp");
					}
				}
				gs1_encoder_setBmp(ctx, i);
				break;
			case 7:
			 if (gs1_encoder_getSym(ctx) == sRSSEXP) {
				printf("\nEnter maximum segments per row. Even values 2 to 22 valid: ");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER 2 THROUGH 22.");
					continue;
				}
				i = atoi(inpStr);
				if (i < 2 || i > 22) {
					printf("OUT OF RANGE. PLEASE ENTER 2 THROUGH 22.");
					continue;
				}
				if (i & 1) {
					printf("ODD NUMBER. PLEASE ENTER AN EVEN NUMBER 2 TO 22.");
					continue;
				}
				gs1_encoder_setSegWidth(ctx, i);
			 }
			 else if ((gs1_encoder_getSym(ctx) == sUCC128_CCA) || (gs1_encoder_getSym(ctx) == sUCC128_CCC)) {
				printf("\nEnter UCC/EAN-128 height in X. 1-%d valid: ",MAX_LINHT);
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER 1 THROUGH %d.",MAX_LINHT);
					continue;
				}
				i = atoi(inpStr);
				if (i < 1 || i > MAX_LINHT) {
					printf("OUT OF RANGE. PLEASE ENTER 1 THROUGH %d.",MAX_LINHT);
					continue;
				}
				gs1_encoder_setLinHeight(ctx, i);
			 }
			 else {
				printf("7 NOT A VALID SELECTION.");
			 }
			 break;
			case 8:
				printf("\nEnter separator row height %d through %d valid: ",
										gs1_encoder_getPixMult(ctx), 2*gs1_encoder_getPixMult(ctx));
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER %d THROUGH %d.",
									gs1_encoder_getPixMult(ctx), 2*gs1_encoder_getPixMult(ctx));
					continue;
				}
				i = atoi(inpStr);
				if (i < gs1_encoder_getPixMult(ctx) || i > 2*gs1_encoder_getPixMult(ctx)) {
					printf("OUT OF RANGE. PLEASE ENTER %d THROUGH %d",
									gs1_encoder_getPixMult(ctx), 2*gs1_encoder_getPixMult(ctx));
					continue;
				}
				gs1_encoder_setSepHt(ctx, i);
				break;
			case 9:
				gs1_encoder_setSym(ctx, sNONE);
				break;
			default:
				printf("OUT OF RANGE. PLEASE ENTER 1 THROUGH 9.");
		}
	}
	return(retFlag);
}

int main(int argc, char *argv[]) {

	gs1_encoder* ctx = gs1_encoder_init();
	if (ctx == NULL) {
		printf("Failed to initialise GS1 Encoders library!\n");
		return 1;
	}

	if (argc == 2 && strcmp(argv[1],"--version") == 0) {
		printf("Application version: " RELEASE "\n");
		printf("Library version: %s\n", gs1_encoder_getVersion(ctx));
		goto out;
	}

	while (userInt(ctx)) {
		if (!gs1_encoder_encode(ctx)) {
			if (gs1_encoder_getErrMsg(ctx)[0] != '\0') printf("\nERROR: %s\n", gs1_encoder_getErrMsg(ctx));
			else printf("\nAn error occurred\n");
			continue;
		}
		printf("\n%s created.", gs1_encoder_getOutFile(ctx));
	}

out:

	gs1_encoder_free(ctx);

	return 0;
}
