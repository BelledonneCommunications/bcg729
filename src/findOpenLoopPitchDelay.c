/*
 findOpenLoopPitchDelay.c

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
#include <string.h>
#include <stdlib.h>

#include "typedef.h"
#include "codecParameters.h"
#include "basicOperationsMacros.h"
#include "utils.h"
#include "g729FixedPointMath.h"

/* local functions prototypes */
/* compute eqA.4 from spec A3.4 on the given range and step(1 compute all the correlation in range, 2 only the even ones) return the maximum and set the index giving it in the first parameter */
word32_t getCorrelationMax(uint16_t *index, word16_t inputSignal[], uint16_t rangeOpen, uint16_t rangeClose, uint16_t step);
/* compute eqA.4 from spec3.4 */
word32_t getCorrelation(word16_t inputSignal[], uint16_t index); 

/*****************************************************************************/
/* findOpenLoopPitchDelay : as specified in specA3.4                         */
/*    paremeters:                                                            */
/*      -(i) weightedInputSignal: 223 values in Q0, buffer                   */
/*           accessed in range [-MAXIMUM_INT_PITCH_DELAY(143), L_FRAME(80)[  */
/*    return value:                                                          */
/*      - the openLoopIntegerPitchDelay in Q0 range [20, 143]                */
/*                                                                           */
/*****************************************************************************/
uint16_t findOpenLoopPitchDelay(word16_t weightedInputSignal[])
{
	int i;
	/*** scale the signal to avoid overflows ***/
	word16_t scaledWeightedInputSignalBuffer[MAXIMUM_INT_PITCH_DELAY+L_FRAME]; /* this buffer might store the scaled version of input Signal, if scaling is not needed, it is not used */
	word16_t *scaledWeightedInputSignal; /* points to the begining of present frame either scaled or directly the input signal */
	word64_t autocorrelation = 0;
	uint16_t indexRange1=0, indexRange2=0, indexRange3Even=0, indexRange3;
	word32_t correlationMaxRange1;
	word32_t correlationMaxRange2;
	word32_t correlationMaxRange3;
	word32_t correlationMaxRange3Odd;
	word32_t autoCorrelationRange1;
	word32_t autoCorrelationRange2;
	word32_t autoCorrelationRange3;
	word32_t normalisedCorrelationMaxRange1;
	word32_t normalisedCorrelationMaxRange2;
	word32_t normalisedCorrelationMaxRange3;
	uint16_t indexMultiple;

	/* compute on 64 bits the autocorrelation on the input signal and if needed scale to have it on 32 bits */
	for (i=-MAXIMUM_INT_PITCH_DELAY; i<L_FRAME; i++) {
		autocorrelation = MAC64(autocorrelation, weightedInputSignal[i], weightedInputSignal[i]);
	}
	if (autocorrelation>MAXINT32) {
		int overflowScale;
		scaledWeightedInputSignal = &(scaledWeightedInputSignalBuffer[MAXIMUM_INT_PITCH_DELAY]);
		overflowScale = PSHR(31-countLeadingZeros((word32_t)(autocorrelation>>31)),1); /* count number of bits needed over the 31 bits allowed and divide by 2 to get the right scaling for the signal */
		for (i=-MAXIMUM_INT_PITCH_DELAY; i<L_FRAME; i++) {
			scaledWeightedInputSignal[i] = SHR(weightedInputSignal[i], overflowScale);
		}

	} else { /* scaledWeightedInputSignal points directly to weightedInputSignal */
		scaledWeightedInputSignal = weightedInputSignal;
	}


	/*** compute the correlationMax in the different ranges ***/
	correlationMaxRange1 = getCorrelationMax(&indexRange1, scaledWeightedInputSignal, 20, 39, 1);
	correlationMaxRange2 = getCorrelationMax(&indexRange2, scaledWeightedInputSignal, 40, 79, 1);
	correlationMaxRange3 = getCorrelationMax(&indexRange3Even, scaledWeightedInputSignal, 80, 143, 2);
	indexRange3 = indexRange3Even;
	/* for the third range, correlationMax shall be computed at +1 and -1 around the maximum found as described in spec A3.4 */
	if (indexRange3>80) { /* don't test value out of range [80, 143] */
		correlationMaxRange3Odd = getCorrelation(scaledWeightedInputSignal, indexRange3-1);
		if (correlationMaxRange3Odd>correlationMaxRange3) {
			correlationMaxRange3 = correlationMaxRange3Odd;
			indexRange3 = indexRange3Even-1;
		}
	}
	correlationMaxRange3Odd = getCorrelation(scaledWeightedInputSignal, indexRange3+1);
	if (correlationMaxRange3Odd>correlationMaxRange3) {
		correlationMaxRange3 = correlationMaxRange3Odd;
		indexRange3 = indexRange3Even+1;
	}

	/*** normalise the correlations ***/
	autoCorrelationRange1 = getCorrelation(&(scaledWeightedInputSignal[-indexRange1]), 0);
	autoCorrelationRange2 = getCorrelation(&(scaledWeightedInputSignal[-indexRange2]), 0);
	autoCorrelationRange3 = getCorrelation(&(scaledWeightedInputSignal[-indexRange3]), 0);
	if (autoCorrelationRange1==0) {
		autoCorrelationRange1 = 1; /* avoid division by 0 */
	}
	if (autoCorrelationRange2==0) {
		autoCorrelationRange2 = 1; /* avoid division by 0 */
	}
	if (autoCorrelationRange3==0) {
		autoCorrelationRange3 = 1; /* avoid division by 0 */
	}

	/* according to ITU code comments, the normalisedCorrelationMax values fit on 16 bits when in Q0, so keep them in Q8 on 32 bits shall not give any overflow */
	normalisedCorrelationMaxRange1 = MULT32_32_Q23(correlationMaxRange1, g729InvSqrt_Q0Q31(autoCorrelationRange1));
	normalisedCorrelationMaxRange2 = MULT32_32_Q23(correlationMaxRange2, g729InvSqrt_Q0Q31(autoCorrelationRange2));
	normalisedCorrelationMaxRange3 = MULT32_32_Q23(correlationMaxRange3, g729InvSqrt_Q0Q31(autoCorrelationRange3));



	/*** Favouring the delays with the values in the lower range ***/
	/* not clearly documented in spec A3.4, algo from the ITU code */
	indexMultiple = SHL(indexRange2,1); /* indexMultiple = 2*indexRange2 */
	if( abs(indexMultiple - indexRange3) < 5) { /* 2*indexRange2 - indexRange3 < 5 */
		normalisedCorrelationMaxRange2 = ADD32(normalisedCorrelationMaxRange2, SHR(normalisedCorrelationMaxRange3,2)); /* Max2 += Max3*0.25 */
	}

	if( abs(indexMultiple + indexRange2 - indexRange3) < 7) { /* 3*indexRange2 - indexRange3 < 5 */
		normalisedCorrelationMaxRange2 = ADD32(normalisedCorrelationMaxRange2, SHR(normalisedCorrelationMaxRange3,2)); /* Max2 += Max3*0.25 */
	}

	indexMultiple = SHL(indexRange1,1); /* indexMultiple = 2*indexRange1 */
	if( abs(indexMultiple - indexRange2) < 5) {  /* 2*indexRange1 - indexRange2 < 5 */
		normalisedCorrelationMaxRange1 = MAC16_32_P15(normalisedCorrelationMaxRange1, O2_IN_Q15, normalisedCorrelationMaxRange2); /* Max1 += Max2*0.2 */
	}

	if( abs(indexMultiple + indexRange1 - indexRange2) < 7) { /* 3*indexRange1 - indexRange2 < 7 */
		normalisedCorrelationMaxRange1 = MAC16_32_P15(normalisedCorrelationMaxRange1, O2_IN_Q15, normalisedCorrelationMaxRange2); /* Max1 += Max2*0.2 */
	}

	/*** return the index corresponding to the greatest normalised Correlation */
	if (normalisedCorrelationMaxRange1<normalisedCorrelationMaxRange2) {
		normalisedCorrelationMaxRange1 = normalisedCorrelationMaxRange2;
		indexRange1 = indexRange2;
	}
	if (normalisedCorrelationMaxRange1<normalisedCorrelationMaxRange3) {
		indexRange1 = indexRange3;
	}
	return indexRange1;
}



/*****************************************************************************/
/* getCorrelation : as specified in specA3.4 eqA.4                           */
/*      correlation = ∑(i=0..39)inputSignal[2*i]*inputSignal[2*i-index]      */
/*    paremeters:                                                            */
/*      -(i) inputSignal: 223 values in Q0, buffer accessed in range         */
/*           [-index, L_FRAME[                                               */
/*      -(i) index: integer value in range [20,143]                          */
/*    return value:                                                          */
/*      -the correlation in Q0 on 32 bits                                    */
/*                                                                           */
/*****************************************************************************/
word32_t getCorrelation(word16_t inputSignal[], uint16_t index) 
{
	int i,j=-index; /* i will be the [2*i] index and j the [2*i-index] in eqA.4 */
	
	word32_t correlation = 0;
	
	for (i=0; i<L_FRAME; i+=2,j+=2) {
		correlation = MAC16_16(correlation, inputSignal[i], inputSignal[j]);
	}

	return correlation;
}

/*****************************************************************************/
/* getCorrelation : compute eqA.4 from spec A3.4 on the given range and      */
/*      step(1 compute all the correlation in range, 2 only the even ones)   */
/*      then return the maximum as specified in specA3.4 eqA.4               */
/*    paremeters:                                                            */
/*      -(o) index : the index giving the maximum of correlation on the      */
/*           considered range                                                */
/*      -(i) inputSignal: signal used to compute the correlation, in Q0      */
/*           accessed in range [-rangeClose, L_FRAME[                        */
/*      -(i) rangeOpen and rangeClose : the index range in which looking for */
/*           the correlation max                                             */
/*      -(i) step : incrementing step for the index                          */
/*    return value :                                                         */
/*      - the correlation maximum found on the given range in Q0 on 32 bits  */
/*                                                                           */
/*****************************************************************************/
word32_t getCorrelationMax(uint16_t *index, word16_t inputSignal[], uint16_t rangeOpen, uint16_t rangeClose, uint16_t step)
{
	int i;
	word32_t correlationMax = MININT32;

	for (i=rangeOpen; i<=rangeClose; i+=step) {
		word32_t correlation = 	getCorrelation(inputSignal, i);
		if (correlation>correlationMax) {
			*index = i;
			correlationMax = correlation;
		}
	}

	return correlationMax;
}
