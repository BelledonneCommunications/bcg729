/*
 utils.c

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
#include "basicOperationsMacros.h"
#include "codecParameters.h"
#include "g729FixedPointMath.h"
#include "codebooks.h"

/*****************************************************************************/
/* insertionSort : sort an array in growing order using insertion algorithm  */
/*    parameters :                                                           */
/*      -(i/o) x: the array to be sorted                                     */
/*      -(i) length: the array length                                        */
/*                                                                           */
/*****************************************************************************/
void insertionSort(word16_t x[], int length) 
{
	int i, j;
	word16_t currentValue;

	for (i=1; i<length; i++) {
		currentValue = x[i];

		j = i-1;
		while ((j>=0) && (x[j]>currentValue)) {
			x[j+1] = x[j];
			j--;
		}

		x[j+1] = currentValue;
	}

	return;
}

/*****************************************************************************/
/* getMinInArray : get the minimum value from an array                       */
/*    parameters :                                                           */
/*      -(i) x: the array to be searched                                     */
/*      -(i) length: the array length                                        */
/*    returns : the minimum value found in the array                         */
/*                                                                           */
/*****************************************************************************/
word16_t getMinInArray(word16_t x[], int length) {
	word16_t min = MAXINT16;
	int i;
	for (i=0; i<length; i++) {
		if (x[i]<min) min = x[i];
	}
	return min;
}

/*****************************************************************************/
/* computeParity : compute parity for pitch delay adaptative codebook index  */
/*      XOR of the 6 MSB (pitchDelay on 8 bits)                              */
/*    parameters :                                                           */
/*      -(i) adaptativeCodebookIndex: the pitch delay on 8 bits              */
/*    return value :                                                         */
/*      the parity bit                                                       */
/*                                                                           */
/*****************************************************************************/
uint16_t computeParity(uint16_t adaptativeCodebookIndex)
{
	int i;
	uint16_t parity = 1;
	adaptativeCodebookIndex = SHR(adaptativeCodebookIndex,2); /* ignore the two LSB */

	for (i=0; i<6; i++) {
		parity ^= adaptativeCodebookIndex&(uint16_t)1; /* XOR with the LSB */
		adaptativeCodebookIndex = SHR(adaptativeCodebookIndex,1); 
	}

	return parity;
}

/*****************************************************************************/
/* rearrangeCoefficients: rearrange coefficients according to spec 3.2.4     */
/*      Have a minimum distance of J beetwen two consecutive coefficients    */
/*    parameters:                                                            */
/*      -(i/o) qLSP: 10 ordered coefficients in Q13 replaced by new values   */
/*             if needed                                                     */
/*      -(i) J: minimum distance between coefficients in Q0.13 (10 or 5)     */
/*                                                                           */
/*****************************************************************************/
void rearrangeCoefficients(word16_t qLSP[], word16_t J)
{ /* qLSP in Q2.13 and J in Q0.13(fitting on 4 bits: possible values 10 and 5) */
	int i=1;
	word16_t delta; /* in Q0.13 */

	for (i=1; i<NB_LSP_COEFF; i++) {
		delta = (ADD16(SUB16(qLSP[i-1],qLSP[i]),J))/2; /* delta = (l[i-1] - l[i] +J)/2 */
		if (delta > 0) {
			qLSP[i-1] = SUB16(qLSP[i-1], delta); /* qLSP still in Q2.13 */
			qLSP[i]   = ADD16(qLSP[i], delta);
		}
	}

	return;
}

/*****************************************************************************/
/* synthesisFilter : compute 1/[A(z)] using the following algorithm          */
/*      filteredSignal[n] = inputSignal[n]                                   */
/*                  - Sum(i=1..10)filterCoefficients[i]*filteredSignal[n-i]  */
/*      for n in [0, L_SUBFRAME[                                             */
/*    parameters:                                                            */
/*      -(i) inputSignal: 40 values in Q0                                    */
/*      -(i) filterCoefficients: 10 coefficients in Q12                      */
/*      -(i/o) filteredSignal: 50 values in Q0 accessed in ranges [-10,-1]   */
/*             as input and [0, 39] as output.                               */
/*                                                                           */
/*****************************************************************************/
void synthesisFilter(word16_t inputSignal[], word16_t filterCoefficients[], word16_t filteredSignal[])
{
	int i;
	for (i=0; i<L_SUBFRAME; i++) {
		word32_t acc = SHL(inputSignal[i],12); /* acc get the first term of the sum, in Q12 (inputSignal is in Q0)*/
		int j;
		for (j=0; j<NB_LSP_COEFF; j++) {
			acc = MSU16_16(acc, filterCoefficients[j], filteredSignal[i-j-1]); /* filterCoefficients in Q12 and signal in Q0 -> acc in Q12 */
		}
		filteredSignal[i] = (word16_t)SATURATE(PSHR(acc, 12), MAXINT16); /* shift right acc to get it back in Q0 and check overflow on 16 bits */
	}

	return;
}

/*****************************************************************************/
/* correlateVectors : compute the correlations between two vectors of        */
/*      L_SUBFRAME length: c[i] = Sum(x[j]*y[j-i]) j in [i, L_SUBFRAME[      */
/*    parameters:                                                            */
/*      -(i) x : L_SUBFRAME length input vector on 16 bits                   */
/*      -(i) y : L_SUBFRAME length input vector on 16 bits                   */
/*      -(o) c : L_SUBFRAME length output vector on 32 bits                  */
/*                                                                           */
/*****************************************************************************/
void correlateVectors (word16_t x[], word16_t y[], word32_t c[])
{
	int i,j;
	for (i=0; i<L_SUBFRAME; i++) {
		c[i] = 0;
		for (j=i; j<L_SUBFRAME; j++) {
			c[i] = MAC16_16(c[i], x[j], y[j-i]);
		}
	}

	return;
}

/*** gain related functions ***/
/*****************************************************************************/
/* MACodeGainPrediction : spec 3.9.1                                         */
/*    parameters:                                                            */
/*      -(i) previousGainPredictionError: qU(m) in eq69 4 values in Q10      */
/*      -(i) fixedCodebookVector: the current subframe fixed codebook vector */
/*           40 values in Q1.13                                              */
/*    return value :                                                         */
/*      - predicted Fixed Codebook gain on 32 bits in Q16 range [3, 1830]    */
/*                                                                           */
/*****************************************************************************/
word32_t MACodeGainPrediction(word16_t *previousGainPredictionError, word16_t *fixedCodebookVector)
{

	/***** algorithm described in spec 3.9.1 *****/
	/*** first compute eq66: E = 10log10((Sum0-39(fixedCodebookVector^2))/40) ***/
	/*** directly used in eq71 in E| - E = 30 - 10log10((Sum0-39(fixedCodebookVector^2))/40) */
	/* = 30     -   10log10((Sum*2^26)/(40*2^26)) */
	/* = 30 + 10log10(40*2^26) - (10/log2(10))*log2(Sum*2^26) [The Sum is computed in Q26] */
	/* = 30 + 94.2884 -  3.0103*log2(Sum*2^26) */
	/* = 124.2884 - 3.0103*log2(Sum*2^26) = E| - E as used in eq71 */

	/* compute the sum of squares of fixedCodebookVector in Q26 */
	int i;
	word32_t fixedCodebookVectorSquaresSum = 0;
	word32_t acc;

	for (i=0; i<L_SUBFRAME; i++) {
		if (fixedCodebookVector[i] != 0) { /* as most of the codebook vector is egal to 0, it worth checking it to avoid useless multiplications */
 			/* fixedCodebookVector in Q1.13 and final sum in range [4, 8.48] (4 values filled with abs values: 1,1,1.8 and 1.8 in the vector give max value of sum) */
			/* fixedCodebookVectorSquaresSum is then in Q4.26 */
			fixedCodebookVectorSquaresSum = MAC16_16(fixedCodebookVectorSquaresSum, fixedCodebookVector[i], fixedCodebookVector[i]);
		}
	}

	/* compute E| - E as in eq71, result in Q16 */
	/* acc = 124.2884 - 3.0103*log2(Sum) */
	/* acc = 8145364[32 bits Q16] - 24660[16 bits Q2.13]*log2(Sum)[32 bits Q16] */
	acc = MAC16_32_Q13(8145364, -24660, g729Log2_Q0Q16(fixedCodebookVectorSquaresSum)); /* acc in Q16 */

	/* accumulate the MA prediction described in eq69 to the previous Sum, result will be in E~(m) + E| -E as used in eq71 */
	/* acc in Q16->Q24, previousGainPredictionError in Q10 and MAPredictionCoefficients in Q0.14*/
	acc = SHL(acc,8); /* acc in Q24 to match the fixed point of next accumulations */
	for (i=0; i<4; i++) {
		acc = MAC16_16(acc, previousGainPredictionError[i], MAPredictionCoefficients[i]); 
	}
	/* acc range [10, 65] -> Q6.24 */
	

	/* compute eq71, we already have the exposant in acc so */
	/* g'c = 10^(acc/20)                                    */
	/*     = 2^((acc*ln(10))/(20*ln(2)))                    */
	/*     = 2^(0,1661*acc)                                 */
	acc = SHR(acc,2); /* Q24->Q22 */
	acc = MULT16_32_Q15(5442, acc); /* 5442 is 0.1661 in Q15 -> acc now in Q4.22 range [1.6, 10.8] */
	acc = PSHR(acc,11); /* get acc in Q4.11 */

	return g729Exp2_Q11Q16((word16_t)acc); /* acc fits on 16 bits, cast it to word16_t to send it to the exp2 function, output in Q16*/
}


/*****************************************************************************/
/* computeGainPredictionError : apply eq72 to compute current fixed Codebook */
/*      gain prediction error and store the result in the adhoc array        */
/*    parameters :                                                           */
/*      -(i) fixedCodebookGainCorrectionFactor: gamma in eq72 in Q3.12       */
/*      -(i/o) previousGainPredictionError: array to be updated in Q10       */
/*                                                                           */
/*****************************************************************************/
void computeGainPredictionError(word16_t fixedCodebookGainCorrectionFactor, word16_t *previousGainPredictionError)
{
	/* need to compute eq72: 20log10(fixedCodebookGainCorrectionFactor) */
	/*  = (20/log2(10))*log2(fixedCodebookGainCorrectionFactor) */
	/*  = 6.0206*log2(fixedCodebookGainCorrectionFactor) */
	/* log2 input in Q0, output in Q16,fixedCodebookGainCorrectionFactor being in Q12, we shall substract 12 in Q16(786432) to the result of log2 function -> final result in Q2.16 */
	word32_t currentGainPredictionError = SUB32(g729Log2_Q0Q16(fixedCodebookGainCorrectionFactor), 786432); 
	currentGainPredictionError = PSHR(MULT16_32_Q12(24660, currentGainPredictionError),6); /* 24660 = 6.0206 in Q3.12 -> mult result in Q16, precise shift right to get it in Q4.10 */
	
	/* shift the array and insert the current Prediction Error */
	previousGainPredictionError[3] = previousGainPredictionError[2];
	previousGainPredictionError[2] = previousGainPredictionError[1];
	previousGainPredictionError[1] = previousGainPredictionError[0];
	previousGainPredictionError[0] = (word16_t)currentGainPredictionError;
}


/*** bitStream to parameters Array conversions functions ***/
/* Note: these functions are in utils.c because used by test source code too */

/*****************************************************************************/
/* parametersArray2BitStream : convert array of parameters to bitStream      */
/*      according to spec 4 - Table 8 and following mapping of values        */
/*               0 -> L0 (1 bit)                                             */
/*               1 -> L1 (7 bits)                                            */
/*               2 -> L2 (5 bits)                                            */
/*               3 -> L3 (5 bits)                                            */
/*               4 -> P1 (8 bit)                                             */
/*               5 -> P0 (1 bits)                                            */
/*               6 -> C1 (13 bits)                                           */
/*               7 -> S1 (4 bits)                                            */
/*               8 -> GA1(3 bits)                                            */
/*               9 -> GB1(4 bits)                                            */
/*               10 -> P2 (5 bits)                                           */
/*               11 -> C2 (13 bits)                                          */
/*               12 -> S2 (4 bits)                                           */
/*               13 -> GA2(3 bits)                                           */
/*               14 -> GB2(4 bits)                                           */
/*    parameters:                                                            */
/*      -(i) parameters : 16 values parameters array                         */
/*      -(o) bitStream : the 16 values streamed on 80 bits in a              */
/*           10*8bits values array                                           */
/*                                                                           */
/*****************************************************************************/
void parametersArray2BitStream(uint16_t parameters[], uint8_t bitStream[])
{
	bitStream[0] = ((parameters[0]&((uint16_t) 0x1))<<7) |
			(parameters[1]&((uint16_t) 0x7f)); 
	
	bitStream[1] = ((parameters[2]&((uint16_t) 0x1f))<<3) |
			((parameters[3]>>2)&((uint16_t) 0x7));

	bitStream[2] = ((parameters[3]&((uint16_t) 0x3))<<6) |
			((parameters[4]>>2)&((uint16_t) 0x3f));

	bitStream[3] = ((parameters[4]&((uint16_t) 0x3))<<6) |
			((parameters[5]&((uint16_t) 0x1))<<5) |
			((parameters[6]>>8)&((uint16_t) 0x1f));

	bitStream[4] = ((parameters[6])&((uint16_t) 0xff));

	bitStream[5] = ((parameters[7]&((uint16_t) 0xf))<<4) |
			((parameters[8]&((uint16_t) 0x7))<<1) |
			((parameters[9]>>3)&((uint16_t) 0x1));

	bitStream[6] = ((parameters[9]&((uint16_t) 0x7))<<5) |
			(parameters[10]&((uint16_t) 0x1f)); 

	bitStream[7] = ((parameters[11]>>5)&((uint16_t) 0xff));

	bitStream[8] = ((parameters[11]&((uint16_t) 0x1f))<<3) |
			((parameters[12]>>1)&((uint16_t) 0x7));

	bitStream[9] = ((parameters[12]&((uint16_t) 0x1))<<7) |
			((parameters[13]&((uint16_t) 0x7))<<4) |
			(parameters[14]&((uint16_t) 0xf));

	return;
}

/*****************************************************************************/
/* CNGparametersArray2BitStream : convert array of parameters to bitStream   */
/*      according to spec B4.3 - Table B2 and following mapping of values    */
/*               0 -> L0 (1 bit)                                             */
/*               1 -> L1 (5 bits)                                            */
/*               2 -> L2 (4 bits)                                            */
/*               3 -> Gain (5 bits)                                          */
/*    parameters:                                                            */
/*      -(i) parameters : 4 values parameters array                          */
/*      -(o) bitStream : the 4 values streamed on 15 bits in a               */
/*           2*8bits values array                                            */
/*                                                                           */
/*****************************************************************************/

void CNGparametersArray2BitStream(uint16_t parameters[], uint8_t bitStream[]) {
	bitStream[0] = ((parameters[0]&((uint16_t) 0x1))<<7) |
			((parameters[1]&((uint16_t) 0x1f))<<2) |
			((parameters[2]>>2)&((uint16_t) 0x3)); 
	
	bitStream[1] = ((parameters[2]&((uint16_t) 0x03))<<6) |
			((parameters[3]&((uint16_t) 0x1f))<<1);
}

/*****************************************************************************/
/* parametersArray2BitStream : convert bitStream to an array of parameters   */
/*             reverse operation of previous funtion                         */
/*    parameters:                                                            */
/*      -(i) bitStream : the 16 values streamed on 80 bits in a              */
/*           10*8bits values array                                           */
/*      -(o) parameters : 16 values parameters array                         */
/*                                                                           */
/*****************************************************************************/
void parametersBitStream2Array(uint8_t bitStream[], uint16_t parameters[])
{
	parameters[0] = (bitStream[0]>>7)&(uint16_t)0x1;
	parameters[1] = bitStream[0]&(uint16_t)0x7f;
	parameters[2] = (bitStream[1]>>3)&(uint16_t)0x1f;
	parameters[3] = (((uint16_t)bitStream[1]&(uint16_t)0x7)<<2) | ((bitStream[2]>>6)&(uint16_t)0x3);
	parameters[4] = (((uint16_t)bitStream[2])&(uint16_t)0x3f)<<2 | ((bitStream[3]>>6)&(uint16_t)0x3);;
	parameters[5] = (bitStream[3]>>5)&(uint16_t)0x1;
	parameters[6] = (((uint16_t)(bitStream[3]&(uint16_t)0x1f))<<8)| bitStream[4];
	parameters[7] = (bitStream[5]>>4)&(uint16_t)0xf;
	parameters[8] = (bitStream[5]>>1)&(uint16_t)0x7;
	parameters[9] = (((uint16_t)bitStream[5]&(uint16_t)0x1)<<3)|((bitStream[6]>>5)&(uint16_t)0x7);
	parameters[10]= (uint16_t)bitStream[6]&(uint16_t)0x1f;
	parameters[11]= (((uint16_t)bitStream[7])<<5)|((bitStream[8]>>3)&(uint16_t)0x1f);
	parameters[12]= ((bitStream[8]&(uint16_t)0x7)<<1) | ((bitStream[9]>>7)&(uint16_t)0x1);
	parameters[13]= (bitStream[9]>>4)&(uint16_t)0x7;
	parameters[14]= bitStream[9]&(uint16_t)0xf;

	return;
}

/*****************************************************************************/
/* pseudoRandom : generate pseudo random number as in spec 4.4.4 eq96        */
/*    parameters:                                                            */
/*      -(i/o) randomGeneratorSeed(updated by this function)                 */
/*    return value :                                                         */
/*      - a unsigned 16 bits pseudo random number                            */
/*                                                                           */
/*****************************************************************************/
uint16_t pseudoRandom(uint16_t *randomGeneratorSeed)
{
	/* pseudoRandomSeed is stored in an uint16_t var, we shall not worry about overflow here */
	/* pseudoRandomSeed*31821 + 13849; */
	*randomGeneratorSeed = MAC16_16(13849, (*randomGeneratorSeed), 31821);
	return *randomGeneratorSeed;
}
