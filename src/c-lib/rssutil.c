/**
 * GS1 barcode encoder library
 *
 * @author Copyright (c) 2000-2021 GS1 AISBL.
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

#include "enc-private.h"
#include "rssutil.h"


/*
 * combins(n,r): returns the number of Combinations of r selected from n:
 *		Combinations = n! /( n-r! * r!)
 *
 */
static int combins(int n, int r) {

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
int *gs1_getRSSwidths(gs1_encoder *ctx, int val, int n, int elements, int maxWidth, int noNarrow)
{
	int *widths = ctx->rss_util_widths;
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


// copies pattern for separator adding 9 narrow elements inside each finder
struct sPrints *gs1_cnvSeparator(gs1_encoder *ctx, struct sPrints *prints)
{
	int i, j, k;
	uint8_t *sepPattern = ctx->rssutil_sepPattern;
	struct sPrints *prntSep = &ctx->rssutil_prntSep;

	prntSep->leftPad = prints->leftPad;
	prntSep->rightPad = prints->rightPad;
	prntSep->reverse = prints->reverse;
	prntSep->pattern = sepPattern;
	prntSep->height = ctx->sepHt;
	prntSep->whtFirst = true;
	prntSep->guards = false;
	for (i = 0, k = 2; k <= 4; k += prints->pattern[i], i++);
	if ((prints->whtFirst && (i&1)==1) || (!prints->whtFirst && (i&1)==0)) {
		sepPattern[0] = 4;
		sepPattern[1] = (uint8_t)(k-4);
		j = 2;
	}
	else {
		sepPattern[0] = (uint8_t)k;
		j = 1;
	}
	for ( ; i < prints->elmCnt; i++, j++) {
		sepPattern[j] = prints->pattern[i];
		if (i < prints->elmCnt - 2 && prints->pattern[i] + prints->pattern[i+1] + prints->pattern[i+2] == 13) {
			if ((j&1)==1) {
				// finder is light/dark/light
				for (k = 0; k < prints->pattern[i]; k++) {
					sepPattern[j+k] = 1; // bwbw... over light
				}
				j += k-1;
				if ((k&1) == 0) {
					i++;
					sepPattern[j] = (uint8_t)(sepPattern[j] + prints->pattern[i]); // trailing w for e1, append to next w
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
					sepPattern[j] = (uint8_t)(sepPattern[j] + prints->pattern[i]); // trailing w for e3, append to next w
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
						sepPattern[j] = (uint8_t)(sepPattern[j] + prints->pattern[i]); // trailing w for e2, append to next w
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
		sepPattern[j-1] = (uint8_t)(k-4);
		sepPattern[j] = 4;
	}
	else {
		j++;
		sepPattern[j] = (uint8_t)k;
	}
	prntSep->elmCnt = j+1;
	return(prntSep);
}
