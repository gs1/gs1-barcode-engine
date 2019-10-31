/***************************************************************************
*
* File: 	RSSEXP.C
*
* Author: 
*
* Date: 	2000-04-02
*
* Description: RSS Expanded encode modules.
*
* History:
*  000510 Fixed 2 segment row bug by replacing lpadr variable with 0 constant
*  010726 Reject ^ (symbol separator) from input string
****************************************************************************/
#include "rssenc.h"

// 2-segments, no guard patterns
#define ELMNTS 21
// 2-segment symbol width in modules, no guard patterns
#define SYM_W		49
// height
#define SYM_H 34
// max double segments
#define MAX_DBL_SEGS 12

#define L_PAD 1 // CC left offset

extern int errFlag;
extern int rowWidth;
extern int line1;
extern int linFlag; // tells pack whether linear or cc is being encoded
extern UCHAR ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

int RSS14Eenc(UCHAR str[], UCHAR pattern[MAX_DBL_SEGS][ELMNTS], int ccFlag);
int symCharPat(UCHAR bars[], int symValue, int parity, int weight,
							 int forwardFlag);
int getVal12(UCHAR bitString[], int symNdx);
int isSymbolSepatator(UCHAR string[]);


void RSSExp(struct sParams *params) {

struct sPrints prints;
struct sPrints chexPrnts;
struct sPrints *prntCnv;

UCHAR linPattern[MAX_DBL_SEGS*ELMNTS+4];
UCHAR chexPattern[MAX_DBL_SEGS*SYM_W+2];
UCHAR dblPattern[MAX_DBL_SEGS][ELMNTS];

int i, j;
int rows, ccFlag;
int segs, lNdx, lNdx1, lMods, lHeight;
int evenRow, rev;
int sepCnt;
int chexSize;
char *ccStr;
int rPadl1, rPadcc;

		ccStr = strchr(params->dataStr, '|');
		if (ccStr == NULL) ccFlag = FALSE;
		else {
			if (params->segWidth < 4) {
				printf("\nComposite must be at least 4 segments wide\n", params->outFile);
				errFlag = TRUE;
				return;
			}
			ccFlag = TRUE;
			ccStr[0] = '\0'; // separate primary data
			ccStr++; // point to secondary data
		}

		rowWidth = params->segWidth; // save for getUnusedBitCnt
		if ((segs = RSS14Eenc(params->dataStr, dblPattern, ccFlag)) > 0) {
			if (errFlag) {
				printf("\nRSS Exp encoding error occurred.");
				return;
			}
			lNdx = 0;
			for (i = 0; i < segs-1; i += 2) {
				for (j = 0; j < 8+5+8; j++) { // copy double segments
					linPattern[lNdx++] = dblPattern[i/2][j];
				}
			}
			if (i == segs-1) {
				for (j = 0; j < 8+5; j++) { // copy last odd segment if one exists
					linPattern[lNdx++] = dblPattern[i/2][j];
				}
			}
			j = (segs <= params->segWidth) ? segs : params->segWidth;
			i = (segs+j-1)/j; // number of linear rows
			lHeight = params->pixMult*i*SYM_H + params->sepHt*(i-1)*3;
			lNdx = (j/2)*(8+5+8) + (j&1)*(8+5);
			lMods = 2 + (j/2)*(17+15+17) + (j&1)*(17+15) + 2;

			// set up checkered seperator pattern and print structure
			for (i = 0; i < MAX_DBL_SEGS*SYM_W+2; i++) {
				chexPattern[i] = 1; // chex = all 1X elements
			}
			chexPattern[0] = 5; // except first and last
			if ((lMods&1) == 0) {
				chexPattern[lMods-8] = 4;
				chexSize = lMods-7;
			}
			else {
				chexPattern[lMods-9] = 5;
				chexSize = lMods-8;
			}
			chexPrnts.elmCnt = chexSize;
			chexPrnts.pattern = &chexPattern[0];
			chexPrnts.guards = FALSE;
			chexPrnts.height = params->sepHt;
			chexPrnts.whtFirst = TRUE;
			chexPrnts.leftPad = 0;
			chexPrnts.rightPad = 0;
			chexPrnts.reverse = FALSE;

			rPadcc = lMods - L_PAD - CCB4_WIDTH;

#if PRNT
			printf("\n%s", params->dataStr);
			printf("\n");
			for (i = 0; i < lNdx; i++) {
				printf("%d", linPattern[i]);
			}
			printf("\n");
#endif
		}
		line1 = TRUE; // so first line is not Y undercut
		if (ccFlag) {
			if ((rows = CC4enc(ccStr, ccPattern)) > 0) {
				if (errFlag) {
					printf("\nComposite encoding error occurred.");
					return;
				}
#if PRNT
				printf("\n%s", ccStr);
				printf("\n");
				for (i = 0; i < rows; i++) {
					for (j = 0; j < CCB4_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
					printf("\n");
				}
#endif
			}
		}

		if (params->bmp) { // BMP version
			// note: BMP is bottom to top inverted
			if (ccFlag) {
				bmpHeader(params->pixMult*(lMods),
								params->pixMult*rows*2 + params->sepHt + lHeight, params->outfp);
			}
			else {
				bmpHeader(params->pixMult*lMods, lHeight, params->outfp);
			}

			// print RSS Exp component
			i = 0;
			evenRow = FALSE; // start with odd
			while (segs > i+params->segWidth) {
				i += params->segWidth;
				evenRow = !evenRow; // alternate row parity
			}

			// print last or only RSS Expanded row
			lNdx1 = ((segs/2)*(8+5+8)+(segs&1)*(8+5)) - ((i/2)*(8+5+8)+(i&1)*8);
			rPadl1 = lMods - 4 -
				(((segs/2)*(17+15+17)+(segs&1)*(17+15)) - ((i/2)*(17+15+17)+(i&1)*17));
			prints.elmCnt = lNdx1;
			prints.pattern = &linPattern[(i/2)*(8+5+8)+(i&1)*8];
			prints.guards = TRUE;
			prints.height = params->pixMult*SYM_H;
			prints.whtFirst = (i/2+1)&1;
			rev = evenRow ^ (i/2)&1;
			if (rev && (((lNdx1-4)%8)&1)) {
				// can't reverse odd # finders so offset it right by one
				prints.leftPad = 1;
				prints.rightPad = rPadl1-1;
				prints.reverse = FALSE;
				// bottom right offset RSS E row
				printElmnts(params, &prints);

				// bottom complement separator
				prntCnv = cnvSeparator(params, &prints);
				printElmnts(params, prntCnv);

				// chex pattern
				printElmnts(params, &chexPrnts);
			}
			else {
				// otherwise normal bottom or only RSS E row
				prints.leftPad = 0;
				prints.rightPad = rPadl1;
				prints.reverse = rev;

				// bottom or only RSS row
				printElmnts(params, &prints);

				if ((i > 0) || (ccFlag)) {
					// CC or lower complement separator
					prntCnv = cnvSeparator(params, &prints);
					printElmnts(params, prntCnv);
				}

				if (i > 0) {
					// chex pattern
					printElmnts(params, &chexPrnts);
				}
			}
			evenRow = !evenRow;

			// print any remaining RSS Expanded rows
			prints.elmCnt = lNdx;
			prints.leftPad = 0;
			prints.rightPad = 0;
			for (i -= params->segWidth ; i >= 0; i -= params->segWidth) {
				j = i + params->segWidth; // last segment number + 1 in this row
				rev = evenRow ^ (i/2)&1;
				prints.pattern = &linPattern[(i/2)*(8+5+8)+(i&1)*8];
				prints.whtFirst = (i/2+1)&1;
				prints.reverse = rev;

				// top complement separator
				prntCnv = cnvSeparator(params, &prints);
				printElmnts(params, prntCnv);

				// upper RSS row
				printElmnts(params, &prints);

				if ((i > 0) || (ccFlag)) {
					// CC or lower complement separator
					prntCnv = cnvSeparator(params, &prints);
					printElmnts(params, prntCnv);
				}

				if (i > 0) {
					// chex pattern
					printElmnts(params, &chexPrnts);
				}
				evenRow = !evenRow;
			}

			if (ccFlag) {
				// print composite component
				prints.elmCnt = CCB4_ELMNTS;
				prints.guards = FALSE;
				prints.height = params->pixMult*2;
				prints.leftPad = L_PAD;
				prints.rightPad = rPadcc;
				prints.whtFirst = TRUE;
				prints.reverse = FALSE;
				for (i = rows-1; i >= 0; i--) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}
		}

		else { // TIFF version
			if (ccFlag) {
				tifHeader(params->pixMult*(lMods),
						params->pixMult*rows*2 + params->sepHt + lHeight, params->outfp);
			}
			else {
				tifHeader(params->pixMult*lMods, lHeight, params->outfp);
			}

			if (ccFlag) {
				// print composite component
				prints.elmCnt = CCB4_ELMNTS;
				prints.guards = FALSE;
				prints.height = params->pixMult*2;
				prints.leftPad = L_PAD;
				prints.rightPad = rPadcc;
				prints.whtFirst = TRUE;
				prints.reverse = FALSE;
				for (i = 0; i < rows; i++) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}

			// print RSS Exp
			evenRow = FALSE; // start with 1st row
			prints.elmCnt = lNdx;
			prints.guards = TRUE;
			prints.height = params->pixMult*SYM_H;
			prints.leftPad = 0;
			prints.rightPad = 0;

			for (i = 0; i < segs-params->segWidth; i += params->segWidth) {
				j = i + params->segWidth; // last segment number + 1 in this row

				rev = evenRow ^ (i/2)&1;
				prints.pattern = &linPattern[(i/2)*(8+5+8)+(i&1)*8];
				prints.whtFirst = (i/2+1)&1;
				prints.reverse = rev;

				if (i > 0) {
					// chex pattern
					printElmnts(params, &chexPrnts);
				}

				if ((i > 0) || (ccFlag)) {
					// CC or lower complement separator
					prntCnv = cnvSeparator(params, &prints);
					printElmnts(params, prntCnv);
				}

				// upper RSS row
				printElmnts(params, &prints);

				// upper complement separator
				prntCnv = cnvSeparator(params, &prints);
				printElmnts(params, prntCnv);

				evenRow = !evenRow;
			}

			// print last or only RSS Expanded row
			lNdx1 = ((segs/2)*(8+5+8)+(segs&1)*(8+5)) - ((i/2)*(8+5+8)+(i&1)*8);
			rPadl1 = lMods - 4 -
				(((segs/2)*(17+15+17)+(segs&1)*(17+15)) - ((i/2)*(17+15+17)+(i&1)*17));
			prints.elmCnt = lNdx1;
			prints.pattern = &linPattern[(i/2)*(8+5+8)+(i&1)*8];
			prints.whtFirst = (i/2+1)&1;
			rev = evenRow ^ (i/2)&1;
			if (rev && (((lNdx1-4)%8)&1)) {
				// can't reverse odd # finders so offset it right by one
				prints.leftPad = 1;
				prints.rightPad = rPadl1-1;
				prints.reverse = FALSE;

				// chex pattern
				printElmnts(params, &chexPrnts);

				// bottom complement separator
				prntCnv = cnvSeparator(params, &prints);
				printElmnts(params, prntCnv);

				// bottom right offset RSS E row
				printElmnts(params, &prints);
			}
			else {
				// otherwise normal row
				prints.leftPad = 0;
				prints.rightPad = rPadl1;
				prints.reverse = rev;

				if (i > 0) {
					// chex pattern
					printElmnts(params, &chexPrnts);
				}

				if ((i > 0) || (ccFlag)) {
					// CC or lower complement separator
					prntCnv = cnvSeparator(params, &prints);
					printElmnts(params, prntCnv);
				}

				// bottom right offset RSS E row
				printElmnts(params, &prints);
			}
			evenRow = !evenRow;
		}
		return;
}


#define PARITY_MOD 211
#define PARITY_PWR 3

// convert AI string to bar widths in dbl segments
int RSS14Eenc(UCHAR string[], UCHAR bars[MAX_DBL_SEGS][ELMNTS], int ccFlag) {

#define FINDER_SIZE 6

static UCHAR finders[FINDER_SIZE][3] = {
	{ 1,8,4 },
	{ 3,6,4 },
	{ 3,4,6 },
	{ 3,2,8 },
	{ 2,6,5 },
	{ 2,2,9 } };



static int finderSets[10][11] = {
	{ 1,	-1,	0,	 0,	0,	 0,	0,	 0,	0,	 0,	0},
	{ 1,	-2,	2,	 0,	0,	 0,	0,	 0,	0,	 0,	0},
	{ 1,	-3,	2,	-4,	0,	 0,	0,	 0,	0,	 0,	0},
	{ 1,	-5,	2,	-4,	3,	 0,	0,	 0,	0,	 0,	0},
	{ 1,	-5,	2,	-4,	4,	-6,	0,	 0,	0,	 0,	0},
	{ 1,	-5,	2,	-4,	5,	-6,	6,	 0,	0,	 0,	0},
	{ 1,	-1,	2,	-2,	3,	-3,	4,	-4,	0,	 0,	0},
	{ 1,	-1,	2,	-2,	3,	-3,	4,	-5,	5,	 0,	0},
	{ 1,	-1,	2,	-2,	3,	-3,	4,	-5,	6,	-6,	0},
	{ 1,	-1,	2,	-2,	3,	-4,	4,	-5,	5,	-6,	6} };

// element 1 weighting for characters N determined by adjacent finder
static int parWts[24] = { 0,1,20,189,193,62,185,113,150,46,76,43,16,109,
												70,134,148,6,120,79,103,161,55,45 };

int i, j;
int parity, weight;
int symValue;
int size, fndrNdx, fndrSetNdx;
UCHAR bitField[MAX_DBL_SEGS*3];

	linFlag = TRUE;
	parity = 0;

	if (((i=check2DData(string)) != 0) || ((i=isSymbolSepatator(string)) != 0)) {
		printf("\nillegal character in RSS Expanded data = '%c'", string[i]);
		errFlag = TRUE;
		return(0);
	}
#if PRNT
	printf("%s\n", string);
#endif
	putBits(bitField, 0, 1, (UINT)ccFlag); // 2D linkage bit
	size = pack(string, bitField);
	if (size < 0) {
		printf("\ndata error");
		errFlag = TRUE;
		return(0);
	}

	// note size is # of data chars, not segments
	if ((bitField[0]&0x40) == 0x40) {
		// method 1, insert variable length symbol bit field
		bitField[0] |= (((size+1)&1)<<5) + ((size > 13)?0x10:0);
	}
	if ((bitField[0]&0x60) == 0) {
		// method 00, insert variable length symbol bit field
		bitField[0] |= (((size+1)&1)<<4) + ((size > 13)?8:0);
	}
	if ((bitField[0]&0x71) == 0x30) {
		// method 01100/01101, insert variable length symbol bit field
		bitField[0] |= (((size+1)&1)<<1) + ((size > 13)?1:0);
	}
	fndrSetNdx = (size - 2) / 2;

	for (i = 0; i < (size+2)/2; i++) { // loop through all dbl segments
		fndrNdx = finderSets[fndrSetNdx][i];
		// fill left data char in dbl segment if not first (check char)
		j = (fndrNdx >= 0) ? fndrNdx*2 : -fndrNdx*2+1;
		if (i > 0) {
			weight = parWts[2*(j-2)];
			symValue = getVal12(bitField, i*2-1);
			parity = symCharPat(&bars[i][0], symValue, parity, weight, TRUE);
		}
		// fill finder for dbl segment
		if (fndrNdx < 0) { // reversed finder
			bars[i][12] = finders[-fndrNdx-1][0];
			bars[i][11] = finders[-fndrNdx-1][1];
			bars[i][10] = finders[-fndrNdx-1][2];
			bars[i][9] = 1;
			bars[i][8] = 1;
		}
		else { // forward finder
			bars[i][8] = finders[fndrNdx-1][0];
			bars[i][9] = finders[fndrNdx-1][1];
			bars[i][10] = finders[fndrNdx-1][2];
			bars[i][11] = 1;
			bars[i][12] = 1;
		}
		// fill right data char in dbl segment if it exists
		if (size > i*2) {
			weight = parWts[2*(j-2)+1];
			symValue = getVal12(bitField, i*2);
			parity = symCharPat(&bars[i][8+5], symValue, parity, weight, FALSE);
		}
	}
	// fill in first parity char
	symCharPat(bars[0], (size-3)*PARITY_MOD + parity, 0, weight, TRUE);
	return(size+1);
}

#define K	4

// Fills in elements for a symbol character given a element array and
// and a symbol char value. Updates the parity *weight. Will fill in
// the array forward or reverse order for odd or even characters.
// Returns the updated parity.
int symCharPat(UCHAR bars[], int symValue, int parity, int weight,
							 int forwardFlag) {

// odd elements N & max, even N & max, odd mul, combos:
static int tbl174[5*6] = {
		/* 17,4 */	12,7,	5,2,	4,	348,
								10,5,	7,4,	20,	1040,
								8,4,	9,5,	52,	1560,
								6,3,	11,6,	104,1040,
								4,1,	13,8,	204,204 };

int i, value, saveVal;
int elementN, elementMax;
int wgtOdd, wgtEven;
int iIndex;
int *widths;

	wgtOdd = weight;
	wgtEven = (weight * 3) % PARITY_MOD;

	// get sym char val index into tbl174
	iIndex = 0;
	while (symValue >= tbl174[iIndex+5]) {
		symValue -= tbl174[iIndex+5];
		iIndex += 6;
	}

	// get odd elements N and max
	elementN = tbl174[iIndex];
	elementMax = tbl174[iIndex+1];
	saveVal = value = symValue / tbl174[iIndex+4];

	// generate and store odd element widths:
	widths = getRSSwidths(value, elementN, K, elementMax, 0);
	for (i = 0; i < 4; i++) {
		if (forwardFlag) {
			bars[i*2] = (UCHAR)widths[i]; // store in 0,2,4,6
		}
		else {
			bars[7 - i*2] = (UCHAR)widths[i]; // else in 7,5,3,1
		}
		parity += wgtOdd * widths[i];
		parity = parity % PARITY_MOD;
		wgtOdd = (wgtOdd * 9) % PARITY_MOD;
	}

	// calculate even elements value:
	value = symValue - (tbl174[iIndex+4] * saveVal);
	elementN = tbl174[iIndex+2];
	elementMax = tbl174[iIndex+3];

	// generate and store even element widths:
	widths = getRSSwidths(value, elementN, K, elementMax, 1);
	for (i = 0; i < 4; i++) {
		if (forwardFlag) {
			bars[1 + i*2] = (UCHAR)widths[i]; // store in 1,3,5,7
		}
		else {
			bars[6 - i*2] = (UCHAR)widths[i]; // else in 6,4,2,0
		}
		parity += wgtEven * widths[i];
		parity = parity % PARITY_MOD;
		wgtEven = (wgtEven * 9) % PARITY_MOD;
	}
	return(parity);
}

// gets the next 12 bit sym char from bit string
int getVal12(UCHAR bitString[], int symNdx) {
int val, ndx;

	ndx = symNdx*3/2; // index into bitString
	if (symNdx & 1) {
		// odd sym char count, so val = 4+8 bits
		val = (int)bitString[ndx] & 0xF;
		val = (val << 8) + (int)bitString[ndx+1];
	}
	else {
		// even sym char count, so val = 8+4 bits
		val = (int)bitString[ndx];
		val = (val << 4) + ((int)bitString[ndx+1] >> 4);
	}
	return(val);
}

// looks for '^' (symbol separator) in string and returns char index iff found
int isSymbolSepatator(UCHAR string[]) {

int i;

	for (i = 0; i < (int)strlen(string); i++) {
		if (string[i] == '^') {
			return(i);
		}
	}
	return(0);
}
