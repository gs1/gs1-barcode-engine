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

#include "rssenc.h"
#include <stdint.h>

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

int combins(int n, int r);

void printElm(int width, int color, int *bits, int * ndx, UCHAR xorMsk);
int userInt(struct sParams *params);
int getSym(struct sParams *params);

// globals
int errFlag;
int rowWidth;
int line1;
int linFlag; // tells pack whether linear or cc is being encoded
UCHAR ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

int main(int argc, char *argv[]) {

// Silence compiler
(void)argc;
(void)argv;

FILE *iFile, *oFile;

struct sParams params;
int i;

	init928();
	initLogTables();
	params.pixMult = 1; // init params
	params.Xundercut = 0;
	params.Yundercut = 0;
	params.sepHt = 1;
	params.bmp = FALSE;
	params.segWidth = 22;
	params.linHeight = 25;
	strcpy(params.outFile, "out.tif");
	params.sym = sNONE;
	params.inputFlag = 0; // for kbd input
	while (userInt(&params)) {
		errFlag = FALSE;
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
				errFlag = TRUE;
		}

		fclose(oFile);
		if (!errFlag) {
			printf("\n%s created.", params.outFile);
		}
	}
	return 0;
}

/**********************************************************************
* getRSSwidths
* routine to generate widths for RSS elements for a given value.
* Calling arguments:
* val = required value
*	n = number of modules
* elements = elements in set (RSS-14 & Expanded = 4; RSS-14 Limited = 7)
*	maxWidth = maximum module width of an element
*	noNarrow = 0 will skip patterns without a narrow element
* Return:
* int widths[] = element widths
************************************************************************/
#define MAX_K	14

int *getRSSwidths(int val, int n, int elements, int maxWidth, int noNarrow)
{
static int widths[MAX_K];
int bar;
int elmWidth;
int mxwElement;
int subVal, lessVal;
int narrowMask = 0;

	for (bar = 0; bar < elements-1; bar++)
	{
		for (elmWidth = 1, narrowMask |= (1<<bar);
				 ;
				 elmWidth++, narrowMask &= ~(1<<bar))
		{
			/* get all combinations */
			subVal = combins(n-elmWidth-1, elements-bar-2);
			/* less combinations with no narrow */
			if ((!noNarrow) && (narrowMask == 0) &&
					 (n-elmWidth-(elements-bar-1) >= elements-bar-1))
			{
				subVal -= combins(n-elmWidth-(elements-bar), elements-bar-2);
			}
			/* less combinations with elements > maxVal */
			if (elements-bar-1 > 1)
			{
				lessVal = 0;
				for (mxwElement = n-elmWidth-(elements-bar-2);
						 mxwElement > maxWidth;
						 mxwElement--)
				{
					lessVal += combins(n-elmWidth-mxwElement-1, elements-bar-3);
				}
				subVal -= lessVal * (elements-1-bar);
			}
			else if (n-elmWidth > maxWidth)
			{
				subVal--;
			}
			val -= subVal;
			if (val < 0) break;
		}
		val += subVal;
		n -= elmWidth;
		widths[bar] = elmWidth;
	}
	widths[bar] = n;
	return(widths);
}

/* combins(n,r): returns the number of Combinations of r selected from n:
*		Combinations = n! /( n-r! * r!) */
int combins(int n, int r) {

int i, j;
int maxDenom, minDenom;
int val;

	if (n-r > r) {
		minDenom = r;
		maxDenom = n-r;
	}
	else {
		minDenom = n-r;
		maxDenom = r;
	}
	val = 1;
	j = 1;
	for (i = n; i > maxDenom; i--) {
		val *= i;
		if (j <= minDenom) {
			val /= j;
			j++;
		}
	}
	for ( ; j <= minDenom; j++) {
		val /= j;
	}
	return(val);
}

void bmpHeader(long xdim, long ydim, FILE *oFile) {

	uint8_t id[2] = {'B','M'};
	struct b_hdr {
		uint32_t fileLength;
		uint32_t const0a;
		uint32_t const3E;
		uint32_t const28;
		uint32_t width;
		uint32_t height;
		uint16_t const1a;
		uint16_t const1b;
		uint32_t const0b;
		uint32_t const0c;
		uint32_t const0d;
		uint32_t const0e;
		uint32_t const0f;
		uint32_t const0g;
		uint32_t const0h;
		uint32_t constFFFFFF;
	};

	struct b_hdr header = { 0,0,0x3E,0x28,0,0,1,1,
													0,0,0,0,0,0,0,0xFFFFFF };

	header.width = (uint32_t)xdim;
	header.height = (uint32_t)ydim;
	header.fileLength = (uint32_t)(0x3E + (((xdim+31)/32)*4) * ydim); // pad rows to 32-bit boundary

	fwrite(&id, sizeof(id), 1, oFile);
	fwrite(&header, sizeof(header), 1, oFile);
}

void tifHeader(long xdim, long ydim, FILE *oFile) {

	struct t_hdr {
		uint8_t endian[2];
		uint16_t version;
		uint32_t diroff;
	};

	struct t_tag {
		uint16_t tag_type;
		uint16_t num_size;
		uint32_t length;
		uint32_t offset;
	};

#define TAG_CNT 14

	struct t_hdr header = { {'I','I'},42,8L };
	short tagnum = TAG_CNT;
	struct t_tag type = { 0xFE, 4, 1L, 0L };
	struct t_tag width = { 0x100, 3, 1L, 6L };
	struct t_tag height = { 0x101 ,3 ,1L, 16L };
	struct t_tag bitsPerSample = { 0x102, 3, 1L, 1L };
	struct t_tag compress = { 0x103, 3, 1L, 1L };
	struct t_tag whiteIs = { 0x106, 3, 1L, 0L };
	struct t_tag thresholding = { 0x107, 3, 1L, 1L };
	struct t_tag stripOffset = { 0x111, 4, 1L, 8L+2+TAG_CNT*12+4+8+8 };
	struct t_tag samplesPerPix = { 0x115, 3, 1L, 1L };
	struct t_tag stripRows = { 0x116, 4, 1L, 16L };
	struct t_tag stripBytes = { 0x117, 4, 1L, 16L };
	struct t_tag xRes = { 0x11A, 5, 1L, 8L+2+TAG_CNT*12+4 };
	struct t_tag yRes = { 0x11B, 5, 1L, 8L+2+TAG_CNT*12+4+8 };
	struct t_tag resUnit = { 0x128, 3, 1L, 3L }; // centimeters
	uint32_t nextdir = 0L;
	uint32_t xResData[2] = { 120L, 1L }; // 120 = 10mils @ 300 dpi
	uint32_t yResData[2] = { 120L, 1L }; // 120 = 10mils @ 300 dpi

	width.offset = (uint32_t)xdim;
	height.offset = (uint32_t)ydim;
	stripRows.offset = (uint32_t)ydim;
	stripBytes.offset = (uint32_t)(((xdim+7)/8) * ydim);
	xResData[1] = 1L; //reduce to 10 mils
	yResData[1] = 1L; //reduce to 10 mils

	fwrite(&header, sizeof(header), 1, oFile);
	fwrite(&tagnum, sizeof(tagnum), 1, oFile);
	fwrite(&type, sizeof(type), 1, oFile);
	fwrite(&width, sizeof(width), 1, oFile);
	fwrite(&height, sizeof(height), 1, oFile);
	fwrite(&bitsPerSample, sizeof(bitsPerSample), 1, oFile);
	fwrite(&compress, sizeof(compress), 1, oFile);
	fwrite(&whiteIs, sizeof(whiteIs), 1, oFile);
	fwrite(&thresholding, sizeof(thresholding), 1, oFile);
	fwrite(&stripOffset, sizeof(stripOffset), 1, oFile);
	fwrite(&samplesPerPix, sizeof(samplesPerPix), 1, oFile);
	fwrite(&stripRows, sizeof(stripRows), 1, oFile);
	fwrite(&stripBytes, sizeof(stripBytes), 1, oFile);
	fwrite(&xRes, sizeof(xRes), 1, oFile);
	fwrite(&yRes, sizeof(yRes), 1, oFile);
	fwrite(&resUnit, sizeof(resUnit), 1, oFile);
	fwrite(&nextdir, sizeof(nextdir), 1, oFile);
	fwrite(&xResData, sizeof(xResData), 1, oFile);
	fwrite(&yResData, sizeof(yResData), 1, oFile);
	return;
}

#define MAX_LINE 6032 // 10 inches wide at 600 dpi
static UCHAR line[MAX_LINE/8 + 1];
static UCHAR lineUCut[MAX_LINE/8 + 1];

void printElmnts(struct sParams *params, struct sPrints *prints) {

#define WHITE 0

int i, bits, width, ndx, white;
UCHAR xorMsk;
int undercut;

	bits = 1;
	ndx = 0;
	if (prints->whtFirst) {
		white = WHITE;
		undercut = params->Xundercut;
	}
	else {
		white = WHITE^1; // invert white if starting black
		undercut = -params->Xundercut; // -undercut if starting black
	}
	if ((prints->reverse) && ((prints->elmCnt & 1) == 0)) {
		white = white^1; // invert if reversed even elements
		undercut = -undercut;
	}
	xorMsk = params->bmp ? 0xFF : 0; // invert BMP bits
	if (line1) {
		for (i = 0; i < MAX_LINE/8; i++) {
			line[i] = xorMsk;
		}
		line1 = FALSE;
	}
	// fill left pad worth of WHITE
	printElm(prints->leftPad*params->pixMult, WHITE , &bits, &ndx, xorMsk);

	// process WHITE/BLACK elements in pairs for undercut
	if (prints->guards) { // print guard pattern
		printElm(params->pixMult + undercut, white , &bits, &ndx, xorMsk);
		printElm(params->pixMult - undercut, (white^1) , &bits, &ndx, xorMsk);
	}
	for(i = 0; i < prints->elmCnt-1; i += 2) {
		if (prints->reverse) {
			width = (int)prints->pattern[prints->elmCnt-1-i]*params->pixMult + undercut;
		}
		else {
			width = (int)prints->pattern[i]*params->pixMult + undercut;
		}
		printElm(width, white , &bits, &ndx, xorMsk);

		if (prints->reverse) {
			width = (int)prints->pattern[prints->elmCnt-2-i]*params->pixMult - undercut;
		}
		else {
			width = (int)prints->pattern[i+1]*params->pixMult - undercut;
		}
		printElm(width, (white^1) , &bits, &ndx, xorMsk);
	}

	// process any trailing odd numbered element with no undercut
	if (i < prints->elmCnt) {
	 if (prints->guards) { // print last element plus guard pattern
		if (prints->reverse) {
			width = (int)prints->pattern[0]*params->pixMult + undercut;
		}
		else {
			width = (int)prints->pattern[i]*params->pixMult + undercut;
		}
		printElm(width, white , &bits, &ndx, xorMsk);

		printElm(params->pixMult - undercut, (white^1) , &bits, &ndx, xorMsk);
		printElm(params->pixMult, white , &bits, &ndx, xorMsk); // last- no undercut
	 }
	 else { // no guard, print last odd without undercut
		if (prints->reverse) {
			width = (int)prints->pattern[0]*params->pixMult;
		}
		else {
			width = (int)prints->pattern[i]*params->pixMult;
		}
		printElm(width, white , &bits, &ndx, xorMsk);
	 }
	}
	else if (prints->guards) { // even number, just print guard pattern
		printElm(params->pixMult + undercut, white , &bits, &ndx, xorMsk);
		printElm(params->pixMult - undercut, (white^1) , &bits, &ndx, xorMsk);
	}
	// fill right pad worth of WHITE
	printElm(prints->rightPad*params->pixMult, WHITE , &bits, &ndx, xorMsk);
	// pad last byte's bits
	if (bits != 1) {
		while ((bits = (bits<<1) + WHITE) <= 0xff);
		lineUCut[ndx] = (UCHAR)(((line[ndx]^xorMsk)&(bits&0xff))^xorMsk); // Y undercut
		line[ndx++] = (UCHAR)((bits&0xff) ^ xorMsk);
		if (ndx > MAX_LINE/8 + 1) {
			printf("\nPrint line too long");
			errFlag = TRUE;
			return;
		}
	}
	if (params->bmp) {
		while ((ndx & 3) != 0) {
			line[ndx++] = 0xFF; // pad to long word boundary for .BMP
			if (ndx >= MAX_LINE/8 + 1) {
				printf("\nPrint line too long");
				errFlag = TRUE;
				return;
			}
		}
	}

	for (i = 0; i < params->Yundercut; i++) {
		fwrite(lineUCut, sizeof(UCHAR), (size_t)ndx, params->outfp);
	}
	for ( ; i < prints->height; i++) {
		fwrite(line, sizeof(UCHAR), (size_t)ndx, params->outfp);
	}
	return;
}

void printElm(int width, int color, int *bits, int *ndx, UCHAR xorMsk) {

int i;

	for (i = 0; i < width; i++) {
		*bits = (*bits<<1) + color;
		if (*bits > 0xff) {
			lineUCut[*ndx] = (UCHAR)(((line[*ndx]^xorMsk)&(*bits&0xff))^xorMsk); // Y undercut
			line[(*ndx)++] = (UCHAR)((*bits&0xff) ^ xorMsk);
			if (*ndx >= MAX_LINE/8 + 1) {
				*ndx = 0;
				printf("\nPrint line too long in graphic line.");
				errFlag = TRUE;
				return;
			}
			*bits = 1;
		}
	}
	return;
}

#define MAX_SEP_ELMNTS (11*21+4) // for 22 segment RSS Exp

static struct sPrints prntSep;
static UCHAR sepPattern[MAX_SEP_ELMNTS];


// copies pattern for separator adding 9 narrow elements inside each finder
struct sPrints *cnvSeparator(struct sParams *params, struct sPrints *prints)
	{
int i, j, k;

	prntSep.leftPad = prints->leftPad;
	prntSep.rightPad = prints->rightPad;
	prntSep.reverse = prints->reverse;
	prntSep.pattern = sepPattern;
	prntSep.height = params->sepHt;
	prntSep.whtFirst = TRUE;
  prntSep.guards = FALSE;
	for (i = 0, k = 2; k <= 4; k += prints->pattern[i], i++);
	if ((prints->whtFirst && (i&1)==1) || (!prints->whtFirst && (i&1)==0)) {
		sepPattern[0] = 4;
		sepPattern[1] = (UCHAR)(k-4);
		j = 2;
	}
	else {
		sepPattern[0] = (UCHAR)k;
		j = 1;
	}
	for ( ; i < prints->elmCnt; i++, j++) {
		sepPattern[j] = prints->pattern[i];
		if (prints->pattern[i] + prints->pattern[i+1] + prints->pattern[i+2] == 13) {
			if ((j&1)==1) {
				// finder is light/dark/light
				for (k = 0; k < prints->pattern[i]; k++) {
					sepPattern[j+k] = 1; // bwbw... over light
				}
				j += k-1;
				if ((k&1) == 0) {
					i++;
					sepPattern[j] = (UCHAR)(sepPattern[j] + prints->pattern[i]); // trailing w for e1, append to next w
				}
				else {
					i++;
					j++;
					sepPattern[j] = prints->pattern[i]; // trailing b, next is w e2
				}
				i++;
				j++;
				for (k = 0; k < prints->pattern[i]; k++) {
					sepPattern[j+k] = 1; // bwbw... over light e3
				}
				j += k-1;
				if ((k&1) == 0) {
					i++;
					sepPattern[j] = (UCHAR)(sepPattern[j] + prints->pattern[i]); // trailing w for e3, append to next w
				}
				else {
					i++;
					j++;
					sepPattern[j] = prints->pattern[i]; // trailing b for e3, next is w
				}
			}
			else {
				// finder is dark/light/dark
				i++;
				if (prints->pattern[i] > 1) {
					j++;
					for (k = 0; k < prints->pattern[i]; k++) {
						sepPattern[j+k] = 1; // bwbw... over light e2
					}
					j += k-1;
					if ((k&1) == 0) {
						i++;
						sepPattern[j] = (UCHAR)(sepPattern[j] + prints->pattern[i]); // trailing w for e2, append to next w
					}
					else {
						i++;
						j++;
						sepPattern[j] = prints->pattern[i]; // trailing b for e2, next is w
					}
				}
				else {
					i++;
					sepPattern[j] = 10; // 1X light e2 (val=3), so w/b/w = 10/1/2
					sepPattern[j+1] = 1;
					sepPattern[j+2] = 2;
					j+=2;
				}
			}
		}
	}
	k = 2;
	j--;
	for ( ; k <= 4; k += sepPattern[j], j--);
	if ((j&1)==0) {
		j += 2;
		sepPattern[j-1] = (UCHAR)(k-4);
		sepPattern[j] = 4;
	}
	else {
		j++;
		sepPattern[j] = (UCHAR)k;
	}
  prntSep.elmCnt = j+1;
	return(&prntSep);
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

int userInt(struct sParams *params) {

int inMenu = TRUE;
int retFlag = TRUE; // return is FALSE if exit program
char inpStr[MAX_KEYDATA+1];
int menuVal, i;

	while (inMenu) {
		if (params->sym == sNONE) {
			if (!getSym(params)) {
				printf("DONE.\n");
				return(FALSE);
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
				return(FALSE);
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
				inMenu = FALSE;
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
				inMenu = FALSE;
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

int getSym(struct sParams *params) {

char inpStr[MAX_KEYDATA+1];
int i;

	while (TRUE) {
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
			return(FALSE);
		}
		if (params->sym > 12) {
			printf("PLEASE ENTER 0 THROUGH 12.");
			continue;
		}
		return(TRUE);
	}
}
