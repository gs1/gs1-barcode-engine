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
#include <stdlib.h>
#include <string.h>
#include "release.h"
#include "enc.h"

#define MAX_PIXMULT 12

#define MAX_KEYDATA 120
#define MAX_LINHT 500 // max UCC/EAN-128 height in X

const char* SYMBOLOGY_NAMES[] =
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

static int getSym(struct sParams *params);
static int userInt(struct sParams *params);

int main(int argc, char *argv[]) {

// Silence compiler
(void)argc;
(void)argv;

FILE *iFile, *oFile;

struct sParams params;
int i;

	encInit();
	params.pixMult = 1; // init params
	params.Xundercut = 0;
	params.Yundercut = 0;
	params.sepHt = 1;
	params.bmp = false;
	params.segWidth = 22;
	params.linHeight = 25;
	strcpy(params.outFile, "out.tif");
	params.sym = sNONE;
	params.inputFlag = 0; // for kbd input
	while (userInt(&params)) {
		errFlag = false;
		if (params.inputFlag == 1) {
			if((iFile = fopen(params.dataFile, "r")) == NULL) {
				printf("\nUNABLE TO OPEN %s FILE\n", params.dataFile);
				continue;
			}
			i = (int)fread(params.dataStr, sizeof(char), MAX_DATA, iFile);
			while (i > 0 && params.dataStr[i-1] < 32) i--; // strip trailing CRLF etc.
			params.dataStr[i] = '\0';
			fclose(iFile);
		}

		if((oFile = fopen(params.outFile, "wb")) == NULL) {
			printf("\nUNABLE TO OPEN %s FILE\n", params.outFile);
			continue;
		}
		params.outfp = oFile;

		switch (params.sym) {
			case sRSS14:
			case sRSS14T:
				RSS14(&params);
				break;

			case sRSS14S:
				RSS14S(&params);
				break;

			case sRSS14SO:
				RSS14SO(&params);
				break;

			case sRSSLIM:
				RSSLim(&params);
				break;

			case sRSSEXP:
				RSSExp(&params);
				break;

			case sUPCA:
			case sEAN13:
				EAN13(&params);
				break;

			case sUPCE:
				UPCE(&params);
				break;

			case sEAN8:
				EAN8(&params);
				break;

			case sUCC128_CCA:
				U128A(&params);
				break;

			case sUCC128_CCC:
				U128C(&params);
				break;

			default:
				printf("\nILLEGAL SYMBOLOGY TYPE %d", params.sym);
				errFlag = true;
		}

		fclose(oFile);
		if (!errFlag) {
			printf("\n%s created.", params.outFile);
		}
	}
	return 0;
}


// Replacement for the deprecated gets(3) function
static char* gets(char* in) {

char* s;

	s = fgets(in,MAX_KEYDATA+1,stdin);
	if (s != NULL) {
		s[strcspn(s, "\r\n")] = 0;
	}
	return s;
}

static int userInt(struct sParams *params) {

int inMenu = true;
int retFlag = true; // return is false if exit program
char inpStr[MAX_KEYDATA+1];
int menuVal, i;

	while (inMenu) {
		if (params->sym == sNONE) {
			if (!getSym(params)) {
				printf("DONE.\n");
				return(false);
			}
		}
		printf("\n\nData input string or file format:");
		switch (params->sym) {
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
		printf("\n\nMENU (Symbology: %s):", SYMBOLOGY_NAMES[params->sym]);
		printf("\n 0) Enter pixels per X. Current value = %d", params->pixMult);
		printf("\n 1) Enter X pixels to undercut. Current value = %d", params->Xundercut);
		printf("\n 2) Enter Y pixels to undercut. Current value = %d", params->Yundercut);
		printf("\n 3) Enter %s output file name. Current name = %s",
							 (params->bmp) ? "BMP":"TIF", params->outFile);
		printf("\n 4) Select keyboard or file input source. Current = %s",
							 (params->inputFlag == 0) ? "keyboard":"file");
		if (params->inputFlag == 0) { // for kbd input
			printf("\n 5) Key enter data input string. %s output file will be created.",
							 (params->bmp) ? "BMP":"TIF");
		}
		else {
			printf("\n 5) Enter data input file name. %s output file will be created.",
							 (params->bmp) ? "BMP":"TIF");
		}
		printf("\n 6) Select TIF or BMP format. Current = %s",
							 (params->bmp) ? "BMP":"TIF");
		if (params->sym == sRSSEXP) {
			printf("\n 7) Select maximum segments per row. Current value = %d", params->segWidth);
		}
		if ((params->sym == sUCC128_CCA) || (params->sym == sUCC128_CCC)) {
			printf("\n 7) Enter GS1-128 height in X. Current value = %d",
								params->linHeight);
		}
		printf("\n 8) Enter separator row height. Current value = %d", params->sepHt);
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
				params->pixMult = i;
				if (i <= params->Xundercut) {
					printf("RESETTING X UNDERCUT TO 0.");
					params->Xundercut = 0;
				}
				if (i <= params->Yundercut) {
					printf("RESETTING Y UNDERCUT TO 0.");
					params->Yundercut = 0;
				}
				if (i*2 < params->sepHt || i > params->sepHt) {
					printf("RESETTING SEPRATOR HEIGHT TO %d.", i);
					params->sepHt = i;
				}
				break;
			case 1:
				if (params->pixMult > 1) {
					printf("\nEnter X pixels to undercut. 0 through %d valid: ",
										params->pixMult-1);
					if (gets(inpStr) == NULL) {
						printf("UNKNOWN. PLEASE ENTER 0 THROUGH %d.",
										params->pixMult-1);
						continue;
					}
					i = atoi(inpStr);
					if (i < 0 || i > params->pixMult-1) {
						printf("OUT OF RANGE. PLEASE ENTER 0 THROUGH %d",
										params->pixMult-1);
						continue;
					}
					params->Xundercut = i;
				}
				else {
					printf("NO UNDERCUT WHEN 1 PIXEL PER X");
				}
				break;
			case 2:
				if (params->pixMult > 1) {
					printf("\nEnter Y pixels to undercut. 0 through %d valid: ",
										params->pixMult-1);
					if (gets(inpStr) == NULL) {
						printf("UNKNOWN. PLEASE ENTER 0 THROUGH %d.",
										params->pixMult-1);
						continue;
					}
					i = atoi(inpStr);
					if (i < 0 || i > params->pixMult-1) {
						printf("OUT OF RANGE. PLEASE ENTER 0 THROUGH %d",
										params->pixMult-1);
						continue;
					}
					params->Yundercut = i;
				}
				else {
					printf("NO UNDERCUT WHEN 1 PIXEL PER X");
				}
				break;
			case 3:
				printf("\nEnter %s output file name with extension: ",
							 (params->bmp) ? "BMP":"TIF");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN ENTRY.");
					continue;
				}
				if (strlen(inpStr) > MAX_FNAME) {
					printf("NOT ACCEPTED. MUST BE 25 CHARACTERS OR FEWER.");
					continue;
				}
				strcpy(params->outFile, inpStr);
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
				params->inputFlag = i;
				break;
			case 5:
			 if (params->inputFlag == 0) { // for kbd input
				printf("\nEnter linear|2d data. No more than %d characters: ", MAX_KEYDATA);
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN ENTRY.");
					continue;
				}
				strcpy(params->dataStr, inpStr);
				inMenu = false;
			 }
			 else {
				printf("\nEnter data input file name: ");
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN ENTRY.");
					continue;
				}
				if (strlen(inpStr) > MAX_FNAME) {
					printf("NOT ACCEPTED. MUST BE 25 CHARACTERS OR FEWER.");
					continue;
				}
				strcpy(params->dataFile, inpStr);
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
				if (params->bmp != i) {
					if (i == 0) {
						strcpy(params->outFile, "out.tif");
					}
					else {
						strcpy(params->outFile, "out.bmp");
					}
				}
				params->bmp = i;
				break;
			case 7:
			 if (params->sym == sRSSEXP) {
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
				params->segWidth = i;
			 }
			 else if ((params->sym == sUCC128_CCA) || (params->sym == sUCC128_CCC)) {
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
				params->linHeight = i;
			 }
			 else {
				printf("7 NOT A VALID SELECTION.");
			 }
			 break;
			case 8:
				printf("\nEnter separator row height %d through %d valid: ",
										params->pixMult, 2*params->pixMult);
				if (gets(inpStr) == NULL) {
					printf("UNKNOWN. PLEASE ENTER %d THROUGH %d.",
									params->pixMult, 2*params->pixMult);
					continue;
				}
				i = atoi(inpStr);
				if (i < params->pixMult || i > 2*params->pixMult) {
					printf("OUT OF RANGE. PLEASE ENTER %d THROUGH %d",
									params->pixMult, 2*params->pixMult);
					continue;
				}
				params->sepHt = i;
				break;
			case 9:
				params->sym = sNONE;
				break;
			default:
				printf("OUT OF RANGE. PLEASE ENTER 1 THROUGH 9.");
		}
	}
	return(retFlag);
}

static int getSym(struct sParams *params) {

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
		params->sym = atoi(inpStr);
		if (params->sym == 0) {
			return(false);
		}
		if (params->sym > 12) {
			printf("PLEASE ENTER 0 THROUGH 12.");
			continue;
		}
		return(true);
	}
}
