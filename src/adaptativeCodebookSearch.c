/*
 adaptativeCodebookSearch.c

 Copyright (C) 2011 Belledonne Communications, Grenoble, France
 Author : Johan Pascal
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "typedef.h"
#include "codecParameters.h"
#include "basicOperationsMacros.h"
#include "utils.h"
#include "codebooks.h"
#include <string.h>

#include "adaptativeCodebookSearch.h"

/*** local functions ***/
void generateAdaptativeCodebookVector(word16_t excitationVector[], int16_t intPitchDelay, int16_t fracPitchDelay);

/*****************************************************************************/
/* adaptativeCodebookSearch: compute parameter P1 and P2 as in spec A.3.7    */
/*     compute also adaptative codebook vector as in spec 3.7.1              */
/*    parameters:                                                            */
/*      -(i/o) excitationVector: [-154,0[ previous excitation as input       */
/*                  Range [0,39[                                             */
/*                  40 words of LPResidualSignal as substitute for current   */
/*                  excitation (spec A.3.7) as input                         */
/*                  40 words of adaptative codebook vector in Q0 as output   */
/*                  Buffer in Q0 accessed in range [-154, 39]                */
/*      -(i/o) intPitchDelayMin: low boundary for pitch delay search         */
/*      -(i/o) intPitchDelayMax: low boundary for pitch delay search         */
/*                  Boundaries are updated during first subframe search      */
/*      -(i) impulseResponse: 40 values as in spec A.3.5 in Q12              */
/*      -(i) targetSignal: 40 values as in spec A.3.6 in Q0                  */
/*                                                                           */
/*      -(o) intPitchDelay: the integer pitch delay                          */
/*      -(o) fracPitchDelay: the fractionnal part of pitch delay             */
/*      -(o) pitchDelayCodeword: P1 or P2 codeword as in spec 3.7.2          */
/*      -(o) adaptativeCodebookVector: 40 words of adaptative codebook vector*/
/*             as described in spec 3.7.1, in Q0.                            */
/*      -(i) subFrameIndex: 0 for the first subframe, 40 for the second      */
/*                                                                           */
/*****************************************************************************/
void adaptativeCodebookSearch(word16_t excitationVector[], int16_t *intPitchDelayMin, int16_t *intPitchDelayMax, word16_t impulseResponse[], word16_t targetSignal[],
				int16_t *intPitchDelay, int16_t *fracPitchDelay, uint16_t *pitchDelayCodeword,  uint16_t subFrameIndex)
{
	int i,j;
	word32_t backwardFilteredTargetSignal[L_SUBFRAME];
	word32_t correlationMax = MININT32;

	/* compute the backward Filtered Target Signal as specified in A.3.7: correlation of target signal and impulse response */
	correlateVectors(targetSignal, impulseResponse, backwardFilteredTargetSignal); /* targetSignal in Q0, impulseResponse in Q12 ->  backwardFilteredTargetSignal in Q12 */
	
	/* maximise the sum as in spec A.3.7, eq A.7 */
	for (i=*intPitchDelayMin; i<=*intPitchDelayMax; i++) {
		word32_t correlation = 0;
		for (j=0; j<L_SUBFRAME; j++) {
			correlation = MAC16_32_Q12(correlation, excitationVector[j-i], backwardFilteredTargetSignal[j]);
		}

		if (correlation>correlationMax) {
			correlationMax=correlation;
			*intPitchDelay = i;
		}
	}

	/* compute the adaptativeCodebookVector (with fracPitchDelay at 0) */
	/* output is in excitationVector[0,L_SUBRAME[ */
	generateAdaptativeCodebookVector(excitationVector, *intPitchDelay, 0);

	/* if we are at first subframe and intPitchDelay >= 85 -> do not compute fracPitchDelay, set it to 0 */
	*fracPitchDelay=0;
	if (!(subFrameIndex==0 && *intPitchDelay>=85)) {
		/* compute the fracPitchDelay*/
		word16_t adaptativeCodebookVector[L_SUBFRAME]; /* as the adaptativeCodebookVector is computed in the excitation vector, use this buffer to backup the one giving the highest numerator */
		word32_t correlation=0;
		/* search the fractionnal part to get the best correlation */
		/* we already have in excitationVector for fracPitchDelay = 0 the adaptativeCodebookVector (see specA.3.7) */
		correlationMax = 0;
		for (i=0; i<L_SUBFRAME; i++) {
			correlationMax = MAC16_32_Q12(correlationMax, excitationVector[i], backwardFilteredTargetSignal[i]);
		}
		/* backup the adaptativeCodebookVector */
		memcpy(adaptativeCodebookVector, excitationVector, L_SUBFRAME*sizeof(word16_t));

		/* Fractionnal part = -1 */
		generateAdaptativeCodebookVector(excitationVector, *intPitchDelay, -1);
		for (i=0; i<L_SUBFRAME; i++) {
			correlation = MAC16_32_Q12(correlation, excitationVector[i], backwardFilteredTargetSignal[i]);
		}
		if (correlation>correlationMax) { /* fractional part at -1 gives higher correlation */
			*fracPitchDelay=-1;
			correlationMax = correlation;
			/* backup the adaptativeCodebookVector */
			memcpy(adaptativeCodebookVector, excitationVector, L_SUBFRAME*sizeof(word16_t));
		}
		
		/* Fractionnal part = 1 */
		generateAdaptativeCodebookVector(excitationVector, *intPitchDelay, 1);
		correlation=0;
		for (i=0; i<L_SUBFRAME; i++) {
			correlation = MAC16_32_Q12(correlation, excitationVector[i], backwardFilteredTargetSignal[i]);
		}
		if (correlation>correlationMax) { /* fractional part at -1 gives higher correlation */
			*fracPitchDelay=1;
		} else { /* previously computed fractional part gives better result */
			/* restore the adaptativeCodebookVector*/
			memcpy(excitationVector, adaptativeCodebookVector, L_SUBFRAME*sizeof(word16_t));
		}
	}

	/* compute the codeword and intPitchDelayMin/intPitchDelayMax if needed (first subframe only) */
	if (subFrameIndex==0) { /* first subframe */
		/* compute intPitchDelayMin/intPitchDelayMax as in spec A.3.7 */
		*intPitchDelayMin = *intPitchDelay - 5;
		if (*intPitchDelayMin < 20) {
			*intPitchDelayMin = 20;
		}
		*intPitchDelayMax = *intPitchDelayMin + 9;
		if (*intPitchDelayMax > MAXIMUM_INT_PITCH_DELAY) {
			*intPitchDelayMax = MAXIMUM_INT_PITCH_DELAY;
			*intPitchDelayMin = MAXIMUM_INT_PITCH_DELAY - 9;
		}

		/* compute the codeword as in spec 3.7.2 */
		if (*intPitchDelay<=85) {
			*pitchDelayCodeword = 3*(*intPitchDelay) - 58 + *fracPitchDelay;
		} else {
			*pitchDelayCodeword = *intPitchDelay + 112;
		}
	} else { /* second subframe */
		/* compute the codeword as in spec 3.7.2 */
		*pitchDelayCodeword = 3*(*intPitchDelay-*intPitchDelayMin) + *fracPitchDelay +2;
	}
}

/*****************************************************************************/
/* generateAdaptativeCodebookVector : according to spec 3.7.1 eq40:          */
/*      generates the adaptative codebook vector by interpolation of past    */
/*      excitation                                                           */
/*    Note : specA.3.7 mention that excitation vector in range [0,39[ being  */
/*      unknown is replaced by LP Residual signal, the ITU code use this     */
/*      buffer to store the adaptative codebok vector and then use it in case*/
/*      of intPitchDelay<40.                                                 */
/*    parameters :                                                           */
/*      -(i/o) excitationVector: in Q0 the past excitation vector accessed   */
/*             [-154,0[. Range [0,39[ is the output in Q0                    */
/*      -(i) intPitchDelay: the integer pitch delay used to access the past  */
/*           excitation                                                      */
/*      -(i) fracPitchDelay: fractional part of the pitch delay: -1, 0 or 1  */
/*                                                                           */
/*****************************************************************************/
void generateAdaptativeCodebookVector(word16_t excitationVector[], int16_t intPitchDelay, int16_t fracPitchDelay)
{
	int n,i,j;
	word16_t *delayedExcitationVector;
	word16_t *b30Increased;
	word16_t *b30Decreased;

	/* fracPitchDelay is in range [-1, 1], convert it to [0,2] needed by eqA.8 */
	fracPitchDelay = -fracPitchDelay;
	if (fracPitchDelay <0) { /* if fracPitchDelay is 1 -> pitchDelay of int+(1/3) -> int+1-(2/3)*/
		intPitchDelay++;
		fracPitchDelay = 2;
	}
	
	/**/
	delayedExcitationVector = &(excitationVector[-intPitchDelay]); /* delayedExcitationVector is used to address the excitation vector at index -intPitchDelay (-k in eq40) */
	b30Increased = &(b30[fracPitchDelay]); /* b30 increased points to b30[fracPitchDelay] : b30[t] in eq40. b30 in Q15 */
	b30Decreased = &(b30[3-fracPitchDelay]); /* b30 decreased points to b30[-fracPitchDelay] : b30[3-t] in eq40. b30 in Q15 */


	for (n=0; n<L_SUBFRAME; n++) {
		word32_t acc = 0; /* acc in Q15 */
		for (i=0, j=0; i<10; i++, j+=3) { /* j is used as a 3*i index */
			acc = MAC16_16(acc, delayedExcitationVector[n-i], b30Increased[j]); /* WARNING: spec 3.7.1 and A.8 give an equation leading to  delayedExcitationVector[n+i] but ITU code uses delayedExcitationVector[n-i], implemented as code */
			acc = MAC16_16(acc, delayedExcitationVector[n+1+i], b30Decreased[j]);
		}
		excitationVector[n] = SATURATE(PSHR(acc, 15), MAXINT16); /* acc in Q15, shift/round to unscaled value and check overflow on 16 bits */
	}
}
