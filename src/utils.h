/*
 utils.h

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
#ifndef UTILS_H
#define UTILS_H

#ifdef _MSC_VER
#define BCG729_INLINE __inline
#else
#define BCG729_INLINE inline
#endif


/*****************************************************************************/
/* insertionSort : sort an array in growing order using insertion algorithm  */
/*    parameters :                                                           */
/*      -(i/o) x: the array to be sorted                                     */
/*      -(i) length: the array length                                        */
/*                                                                           */
/*****************************************************************************/
void insertionSort(word16_t x[], int length);

/*****************************************************************************/
/* getMinInArray : get the minimum value from an array                       */
/*    parameters :                                                           */
/*      -(i) x: the array to be searched                                     */
/*      -(i) length: the array length                                        */
/*    returns : the minimum value found in the array                         */
/*                                                                           */
/*****************************************************************************/
word16_t getMinInArray(word16_t x[], int length);

/*****************************************************************************/
/* computeParity : compute parity for pitch delay adaptative codebook index  */
/*      XOR of the 6 MSB (pitchDelay on 8 bits)                              */
/*    parameters :                                                           */
/*      -(i) adaptativeCodebookIndex: the pitch delay on 8 bits              */
/*    return value :                                                         */
/*      the parity bit                                                       */
/*                                                                           */
/*****************************************************************************/
uint16_t computeParity(uint16_t adaptativeCodebookIndex);

/*****************************************************************************/
/* rearrangeCoefficients: rearrange coefficients according to spec 3.2.4     */
/*      Have a minimum distance of J beetwen two consecutive coefficients    */
/*    parameters:                                                            */
/*      -(i/o) qLSP: 10 ordered coefficients in Q13 replaced by new values   */
/*             if needed                                                     */
/*      -(i) J: minimum distance between coefficients in Q0.13 (10 or 5)     */
/*                                                                           */
/*****************************************************************************/
void rearrangeCoefficients(word16_t qLSP[], word16_t J);

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
void synthesisFilter(word16_t inputSignal[], word16_t filterCoefficients[], word16_t filteredSignal[]);

/*****************************************************************************/
/* correlateVectors : compute the correlations between two vectors of        */
/*      L_SUBFRAME length: c[i] = Sum(x[j]*y[j-i]) j in [i..L_SUBFRAME]      */
/*    parameters:                                                            */
/*      -(i)  x : L_SUBFRAME length input vector on 16 bits                  */
/*      -(i)  y : L_SUBFRAME length input vector on 16 bits                  */
/*      -(o)  c : L_SUBFRAME length output vector on 32 bits                 */
/*                                                                           */
/*****************************************************************************/
void correlateVectors (word16_t x[], word16_t y[], word32_t c[]);

/*****************************************************************************/
/* countLeadingZeros : return the number of zero heading the argument        */
/*      MSB is excluded as considered sign bit.                              */
/*      May be replaced by one asm instruction.                              */
/*    parameters :                                                           */
/*      -(i) x : 32 bits values >= 0 (no checking done by this function)     */
/*    return value :                                                         */
/*      - number of heading zeros(MSB excluded. Ex: 0x0080 00000 returns 7)  */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE uint16_t countLeadingZeros(word32_t x)
{
	uint16_t leadingZeros = 0;
	if (x==0) return 31;
	while (x<(word32_t)0x40000000) {
		leadingZeros++;
		x <<=1;
	}
	return leadingZeros;
}

/*****************************************************************************/
/* unsignedCountLeadingZeros : return the number of zero heading the argument*/
/*      May be replaced by one asm instruction.                              */
/*    parameters :                                                           */
/*      -(i) x : 32 bits unsigned int values                                 */
/*    return value :                                                         */
/*      - number of heading zeros(MSB included. Ex: 0x0080 00000 returns 8)  */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE uint16_t unsignedCountLeadingZeros(uword32_t x)
{
	uint16_t leadingZeros = 0;
	if (x==0) return 32;
	while ((x&0x80000000)!=(uword32_t)0x80000000) {
		leadingZeros++;
		x <<=1;
	}
	return leadingZeros;
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
word32_t MACodeGainPrediction(word16_t *previousGainPredictionError, word16_t *fixedCodebookVector);

/*****************************************************************************/
/* computeGainPredictionError : apply eq72 to compute current fixed Codebook */
/*      gain prediction error and store the result in the adhoc array        */
/*    parameters :                                                           */
/*      -(i) fixedCodebookGainCorrectionFactor: gamma in eq72 in Q3.12       */
/*      -(i/o) previousGainPredictionError: array to be updated in Q10       */
/*                                                                           */
/*****************************************************************************/
void computeGainPredictionError(word16_t fixedCodebookGainCorrectionFactor, word16_t *previousGainPredictionError);

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
void parametersArray2BitStream(uint16_t parameters[], uint8_t bitStream[]);

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
void CNGparametersArray2BitStream(uint16_t parameters[], uint8_t bitStream[]);

/*****************************************************************************/
/* parametersArray2BitStream : convert bitStream to an array of parameters   */
/*             reverse operation of previous funtion                         */
/*    parameters:                                                            */
/*      -(i) bitStream : the 16 values streamed on 80 bits in a              */
/*           10*8bits values array                                           */
/*      -(o) parameters : 16 values parameters array                         */
/*                                                                           */
/*****************************************************************************/
void parametersBitStream2Array(uint8_t bitStream[], uint16_t parameters[]);

/*****************************************************************************/
/* pseudoRandom : generate pseudo random number as in spec 4.4.4 eq96        */
/*    parameters:                                                            */
/*      -(i/o) randomGeneratorSeed(updated by this function)                 */
/*    return value :                                                         */
/*      - a unsigned 16 bits pseudo random number                            */
/*                                                                           */
/*****************************************************************************/
uint16_t pseudoRandom(uint16_t *randomGeneratorSeed);
#endif /* ifndef UTILS_H */
