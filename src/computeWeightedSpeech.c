/*
 computeWeightedSpeech.c

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

#include "computeWeightedSpeech.h"

/*****************************************************************************/
/* computeWeightedSpeech: compute wieghted speech according to spec A3.3.3   */
/*    parameters:                                                            */
/*      -(i) inputSignal : 90 values buffer accessed in range [-10, 79] in Q0*/
/*      -(i) qLPCoefficients: 20 coefficients(10 for each subframe) in Q12   */
/*      -(i) weightedqLPCoefficients: 20 coefficients(10 for each subframe)  */
/*           in Q12                                                          */
/*      -(i/o) weightedInputSignal: 90 values in Q0: [-10, -1] as input      */
/*             [0,79] as output in Q0                                        */
/*      -(o) LPResidualSignal: 80 values of residual signal in Q0            */
/*                                                                           */
/*****************************************************************************/
void computeWeightedSpeech(word16_t inputSignal[], word16_t qLPCoefficients[], word16_t weightedqLPCoefficients[], word16_t weightedInputSignal[], word16_t LPResidualSignal[])
{
	/* algo as specified in A3.3.3: */
	/* first compute LPResidualSignal[n] = inputSignal[n] + ∑(i=1..10)qLP[i]*inputSignal[n-i] specA3.3.3 eqA.3 */
	/* then compute qLP' coefficients as qLP'[i] = weightedqLP[i] - 0.7*weightedqLP[i-1] */

	/* finally get the weightedInputSignal[n] = LPResidualSignal[n] - ∑(i=1..10)qLP'[i]*weightedInputSignal[n-i] spec A3.3.3 eqA.2 */

	int i,j;
	word16_t weightedqLPLowPassCoefficients[NB_LSP_COEFF]; /* in Q12 */

	/*** compute LPResisualSignal (spec A3.3.3 eqA.3) in Q0 ***/
	/* compute residual signal for the first subframe: use the first 10 qLPCoefficients */
	for (i=0; i<L_SUBFRAME; i++) {
		word32_t acc = SHL((word32_t)inputSignal[i], 12); /* inputSignal in Q0 is shifted to set acc in Q12 */
		for (j=0; j<NB_LSP_COEFF; j++) {
			acc = MAC16_16(acc, qLPCoefficients[j],inputSignal[i-j-1]); /* qLPCoefficients in Q12, inputSignal in Q0 -> acc in Q12 */
		}
		LPResidualSignal[i] = (word16_t)SATURATE(PSHR(acc, 12), MAXINT16); /* shift back acc to Q0 and saturate it to avoid overflow when going back to 16 bits */
	}
	/* compute residual signal for the second subframe: use the second part of qLPCoefficients */
	for (i=L_SUBFRAME; i<L_FRAME; i++) {
		word32_t acc = SHL((word32_t)inputSignal[i], 12); /* inputSignal in Q0 is shifted to set acc in Q12 */
		for (j=0; j<NB_LSP_COEFF; j++) {
			acc = MAC16_16(acc, qLPCoefficients[NB_LSP_COEFF+j],inputSignal[i-j-1]); /* qLPCoefficients in Q12, inputSignal in Q0 -> acc in Q12 */
		}
		LPResidualSignal[i] = (word16_t)SATURATE(PSHR(acc, 12), MAXINT16); /* shift back acc to Q0 and saturate it to avoid overflow when going back to 16 bits */
	}

	/*** compute weightedqLPLowPassCoefficients and weightedInputSignal for first subframe ***/
	/* spec A3.3.3 a' = weightedqLPLowPassCoefficients[i] =  weightedqLP[i] - 0.7*weightedqLP[i-1] */
	weightedqLPLowPassCoefficients[0] = SUB16(weightedqLPCoefficients[0],O7_IN_Q12); /* weightedqLP[-1] = 1 -> weightedqLPLowPassCoefficients[0] =  weightedqLPCoefficients[0] - 0.7 */
	for (i=1; i<NB_LSP_COEFF; i++) {
		weightedqLPLowPassCoefficients[i] = SUB16(weightedqLPCoefficients[i], MULT16_16_Q12(weightedqLPCoefficients[i-1], O7_IN_Q12));
	}

	/* weightedInputSignal for the first subframe: synthesis filter  1/[A'(z)] */
	synthesisFilter(LPResidualSignal, weightedqLPLowPassCoefficients, weightedInputSignal);

	/*** compute weightedqLPLowPassCoefficients and weightedInputSignal for second subframe ***/
	/* spec A3.3.3 a' = weightedqLPLowPassCoefficients[i] =  weightedqLP[i] - 0.7*weightedqLP[i-1] */
	weightedqLPLowPassCoefficients[0] = SUB16(weightedqLPCoefficients[NB_LSP_COEFF],O7_IN_Q12); /* weightedqLP[-1] = 1 -> weightedqLPLowPassCoefficients[0] =  weightedqLPCoefficients[0] - 0.7 */
	for (i=1; i<NB_LSP_COEFF; i++) {
		weightedqLPLowPassCoefficients[i] = SUB16(weightedqLPCoefficients[NB_LSP_COEFF+i], MULT16_16_Q12(weightedqLPCoefficients[NB_LSP_COEFF+i-1], O7_IN_Q12));
	}

	/* weightedInputSignal for the second subframe: synthesis filter  1/[A'(z)] */
	synthesisFilter(&(LPResidualSignal[L_SUBFRAME]), weightedqLPLowPassCoefficients, &(weightedInputSignal[L_SUBFRAME]));
}
