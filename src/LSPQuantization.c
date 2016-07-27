/*
 LSPQuantization.c

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
#include "g729FixedPointMath.h"
#include "codebooks.h"

#include "LSPQuantization.h"
#include "string.h"

/* static buffers */
word16_t previousqLSFInit[NB_LSP_COEFF] = {2339, 4679, 7018, 9358, 11698, 14037, 16377, 18717, 21056, 23396}; /* PI*(float)(j+1)/(float)(M+1) */

/* initialise the stactic buffers */
void initLSPQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext)
{
	int i;
	for (i=0; i<MA_MAX_K; i++) {
		memcpy(encoderChannelContext->previousqLSF[i], previousqLSFInit, NB_LSP_COEFF*sizeof(word16_t));
	}
	return;
}
/**********************************************************************************/
/* noiseLSPQuantization : Convert LSP to LSF, Quantize LSF and find L parameters, */
/*      qLSF->qLSP as described in spec A3.2.4                                    */
/*    parameters:                                                                 */
/*      -(i/o) previousqLSF : 4 previousqLSF, is updated by this function         */
/*      -(i) LSPCoefficients : 10 LSP coefficients in Q15                         */
/*      -(o) qLSPCoefficients : 10 qLSP coefficients in Q15                       */
/*      -(o) parameters : 3 parameters L0, L1, L2                                 */
/*                                                                                */
/**********************************************************************************/
void noiseLSPQuantization(word16_t previousqLSF[MA_MAX_K][NB_LSP_COEFF], word16_t LSPCoefficients[], word16_t qLSPCoefficients[], uint8_t parameters[])
{
	int i,j;
	int L0;
	word16_t LSF[NB_LSP_COEFF]; /* LSF coefficients in Q2.13 range [0, Pi[ */
	word16_t weights[NB_LSP_COEFF]; /* weights in Q11 */
	word16_t weightsThreshold[NB_LSP_COEFF]; /* store in Q13 the threshold used to compute the weights */
	word16_t L1index[L0_RANGE];
	word16_t L2index[L0_RANGE];
	word32_t weightedMeanSquareError[L0_RANGE];
	word16_t quantizerOutput[NB_LSP_COEFF];
	word16_t qLSF[NB_LSP_COEFF];

	/*** compute LSF in Q2.13 : lsf = arcos(lsp) range [0, Pi[ spec 3.2.4 eq18 ***/
	for (i=0; i<NB_LSP_COEFF; i++)  {
		LSF[i] = g729Acos_Q15Q13(LSPCoefficients[i]);
	}

	/*** compute the weights vector as in spec 3.2.4 eq22 ***/
	weightsThreshold[0] = SUB16(LSF[1], OO4PIPLUS1_IN_Q13);
	for (i=1; i<NB_LSP_COEFF-1; i++) {
		weightsThreshold[i] = SUB16(SUB16(LSF[i+1], LSF[i-1]), ONE_IN_Q13);
	}
	weightsThreshold[NB_LSP_COEFF-1] = SUB16(O92PIMINUS1_IN_Q13, LSF[NB_LSP_COEFF-2]);

	for (i=0; i<NB_LSP_COEFF; i++) {
		if (weightsThreshold[i]>0) {
			weights[i] = ONE_IN_Q11;
		} else {
			weights[i] = (word16_t)SATURATE(ADD32(PSHR(MULT16_16(MULT16_16_Q13(weightsThreshold[i],  weightsThreshold[i]), 10), 2), ONE_IN_Q11), MAXINT16);
		}
	}
	weights[4] = MULT16_16_Q14(weights[4], ONE_POINT_2_IN_Q14);
	weights[5] = MULT16_16_Q14(weights[5], ONE_POINT_2_IN_Q14);

	/*** compute the coefficients for the two noise noise MA Predictors ***/
	for (L0=0; L0<L0_RANGE; L0++) {
		/* compute the target Vector (l) to be quantized as in spec 3.2.4 eq23 */
		word16_t targetVector[NB_LSP_COEFF]; /* vector to be quantized in Q13 */
		word32_t meanSquareDiff = MAXINT32;
		word16_t quantizedVector[NB_LSP_COEFF]; /* in Q13, the current state of quantized vector */

		for (i=0; i<NB_LSP_COEFF; i++) {
			word32_t acc = SHL(LSF[i],15); /* acc in Q2.28 */
			for (j=0; j<MA_MAX_K; j++) {
				acc = MSU16_16(acc, previousqLSF[j][i], noiseMAPredictor[L0][j][i]); /* previousqLSF in Q2.13 and MAPredictor in Q0.15-> acc in Q2.28 */
			}
			targetVector[i] = MULT16_16_Q12((word16_t)PSHR(acc, 15), invNoiseMAPredictorSum[L0][i]); /* acc->Q13 and invMAPredictorSum in Q12 -> targetVector in Q13 */
		}

		/* find closest match for predictionError (minimize mean square diff) in L1 subset codebook: 32 entries from L1 codebook */
		for (i=0; i<NOISE_L1_RANGE; i++) {
			word32_t acc = 0;
			for (j=0; j<NB_LSP_COEFF; j++) {
				word16_t difftargetVectorL1 = SATURATE(SUB32(targetVector[j], L1[L1SubsetIndex[i]][j]), MAXINT16);
				acc = MAC16_16(acc, difftargetVectorL1, difftargetVectorL1);
			}

			if (acc<meanSquareDiff) {
				meanSquareDiff = acc;
				L1index[L0] = i;
			}
		}

		/* find the closest match in L2 subset wich will minimise the weighted sum of (targetVector - L1 result - L2)^2 */
		/* using eq20, eq21 and eq23 in spec 3.2.4 -> l[i] - l^[i] = (wi - w^[i])/(1-SumMAPred[i]) but ITU code ignores this denominator */
		/* works on the first five coefficients only */
		meanSquareDiff = MAXINT32;
		for (i=0; i<NOISE_L2_RANGE; i++) {
			word32_t acc = 0;
			for (j=0; j<NB_LSP_COEFF/2; j++) {
				/* commented code : compute in the same way of the ITU code: ignore the denonimator and minimize (wi - w^[i])/(1-SumMAPred[i]) instead of (wi - w^[i]) square sum */
				word16_t difftargetVectorL1L2 = SATURATE(MULT16_16_Q15(SUB32(SUB32(targetVector[j], L1[L1SubsetIndex[L1index[L0]]][j]), L2L3[L2SubsetIndex[i]][j]), noiseMAPredictorSum[L0][j]), MAXINT16); /* targetVector, L1 and L2L3 in Q13 -> result in Q13 */
				acc = MAC16_16(acc, difftargetVectorL1L2, MULT16_16_Q11(difftargetVectorL1L2, weights[j])); /* weights in Q11, diff in Q13 */
			}

			for (j=NB_LSP_COEFF/2; j<NB_LSP_COEFF; j++) {
				word16_t difftargetVectorL1L3 = SATURATE(MULT16_16_Q15(SUB32(SUB32(targetVector[j], L1[L1SubsetIndex[L1index[L0]]][j]), L2L3[L3SubsetIndex[i]][j]), noiseMAPredictorSum[L0][j]), MAXINT16); /* targetVector, L1 and L2L3 in Q13 -> result in Q13 */
				acc = MAC16_16(acc, difftargetVectorL1L3, MULT16_16_Q11(difftargetVectorL1L3, weights[j])); /* weights in Q11, diff in Q13 */
			}


			if (acc<meanSquareDiff) {
				meanSquareDiff = acc;
				L2index[L0] = i;
			}
		}

		/* compute the quantized vector L1+L2/L3 and rearrange it as specified in spec 3.2.4 */
		/* Note: according to the spec, the rearrangement shall be done on each candidate while looking for best match, but the ITU code does it after picking the best match and so we do */
		for (i=0; i<NB_LSP_COEFF/2; i++) {
			quantizedVector[i] = ADD16(L1[L1SubsetIndex[L1index[L0]]][i], L2L3[L2SubsetIndex[L2index[L0]]][i]);
		}
		for (i=NB_LSP_COEFF/2; i<NB_LSP_COEFF; i++) {
			quantizedVector[i] = ADD16(L1[L1index[L0]][i], L2L3[L3SubsetIndex[L2index[L0]]][i]);
		}

		/* rearrange with a minimum distance of 0.0012 */
		for (i=1; i<NB_LSP_COEFF/2; i++) {
			if (quantizedVector[i-1]>SUB16(quantizedVector[i],GAP1)) {
				quantizedVector[i-1] = PSHR(SUB16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
				quantizedVector[i] = PSHR(ADD16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
			}
		}
		for (i=NB_LSP_COEFF/2+1; i<NB_LSP_COEFF; i++) {
			if (quantizedVector[i-1]>SUB16(quantizedVector[i],GAP1)) {
				quantizedVector[i-1] = PSHR(SUB16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
				quantizedVector[i] = PSHR(ADD16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
			}
		}

		/* rearrange the whole quantizedVector with a distance of 0.0006 */
		for (i=1; i<NB_LSP_COEFF; i++) {
			if (quantizedVector[i-1]>SUB16(quantizedVector[i],GAP2)) {
				quantizedVector[i-1] = PSHR(SUB16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP2), 1);
				quantizedVector[i] = PSHR(ADD16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP2), 1);
			}
		}

		/* compute the weighted mean square distance using the final quantized vector according to eq21 */
		weightedMeanSquareError[L0]=0;
		for (i=0; i<NB_LSP_COEFF; i++) {
			word16_t difftargetVectorQuantizedVector = SATURATE(MULT16_16_Q15(SUB32(targetVector[i], quantizedVector[i]), noiseMAPredictorSum[L0][i]), MAXINT16); /* targetVector and quantizedVector in Q13 -> result in Q13 */
			weightedMeanSquareError[L0] = MAC16_16(weightedMeanSquareError[L0], difftargetVectorQuantizedVector, MULT16_16_Q11(difftargetVectorQuantizedVector, weights[i])); /* weights in Q11, diff in Q13 */
		}


	}

	/* now select L0 and copy the selected coefficients to the output buffer */
	if (weightedMeanSquareError[0]<weightedMeanSquareError[1]) {
		parameters[0] = 0;
		parameters[1] = L1index[0];
		parameters[2] = L2index[0];
	} else {
		parameters[0] = 1;
		parameters[1] = L1index[1];
		parameters[2] = L2index[1];
	}

	/*** Compute the quantized LSF from the L coefficients ***/
	/* reconstruct vector from the codebooks using the selected parameters spec 3.2.4 eq19 */
	for (i=0; i<NB_LSP_COEFF/2; i++) {
		quantizerOutput[i] = ADD16(L1[L1SubsetIndex[parameters[1]]][i], L2L3[L2SubsetIndex[parameters[2]]][i]); /* codebooks are in Q2.13 for L1 and Q0.13 for L2L3, due to actual values stored in the codebooks, result in Q2.13 */
	}
	for ( i=NB_LSP_COEFF/2; i<NB_LSP_COEFF; i++) {
		quantizerOutput[i] = ADD16(L1[L1SubsetIndex[parameters[1]]][i], L2L3[L3SubsetIndex[parameters[2]]][i]); /* same as previous, output in Q2.13 */
	}
	/* rearrange in order to have a minimum distance between two consecutives coefficients spec 3.2.4 */
	rearrangeCoefficients(quantizerOutput, GAP1);
	rearrangeCoefficients(quantizerOutput, GAP2); /* currentqLSF still in Q2.13 */

	/* compute qLSF spec 3.2.4 eq20 */
	for (i=0; i<NB_LSP_COEFF; i++) {
		word32_t acc = MULT16_16(noiseMAPredictorSum[parameters[0]][i], quantizerOutput[i]); /* (1 - ∑Pi,k)*lˆi(m) Q15 * Q13 -> Q28 */
		for (j=0; j<MA_MAX_K; j++) {
			acc = MAC16_16(acc, noiseMAPredictor[parameters[0]][j][i], previousqLSF[j][i]);
		}
		/* acc in Q2.28, shift back the acc to a Q2.13 with rounding */
		qLSF[i] = (word16_t)PSHR(acc, 15); /* qLSF in Q2.13 */
	}

	/* update the previousqLSF buffer with current quantizer output */
	for (i=MA_MAX_K-1; i>0; i--) {
		memcpy(previousqLSF[i], previousqLSF[i-1], NB_LSP_COEFF*sizeof(word16_t));
	}
	memcpy(previousqLSF[0], quantizerOutput, NB_LSP_COEFF*sizeof(word16_t));

	/*** qLSF stability check ***/
	insertionSort(qLSF, NB_LSP_COEFF);

	/* check for low limit on qLSF[0] */
	if (qLSF[1]<qLSF_MIN) {
		qLSF[1] = qLSF_MIN;
	}

	/* check and rectify minimum distance between two consecutive qLSF */
	for (i=0; i<NB_LSP_COEFF-1; i++) {
		if (SUB16(qLSF[i+1],qLSF[i])<MIN_qLSF_DISTANCE) {
			qLSF[i+1] = qLSF[i]+MIN_qLSF_DISTANCE;
		}
	}

	/* check for upper limit on qLSF[NB_LSP_COEFF-1] */
	if (qLSF[NB_LSP_COEFF-1]>qLSF_MAX) {
		qLSF[NB_LSP_COEFF-1] = qLSF_MAX;
	}

	/* convert qLSF to qLSP: qLSP = cos(qLSF) */
	for (i=0; i<NB_LSP_COEFF; i++) {
		qLSPCoefficients[i] = g729Cos_Q13Q15(qLSF[i]); /* ouput in Q0.15 */
	}

	return;
}

/*****************************************************************************/
/* LSPQuantization : Convert LSP to LSF, Quantize LSF and find L parameters, */
/*      qLSF->qLSP as described in spec A3.2.4                               */
/*    parameters:                                                            */
/*      -(i/o) encoderChannelContext : the channel context data              */
/*      -(i) LSPCoefficients : 10 LSP coefficients in Q15                    */
/*      -(o) qLSPCoefficients : 10 qLSP coefficients in Q15                  */
/*      -(o) parameters : 4 parameters L0, L1, L2, L3                        */
/*                                                                           */
/*****************************************************************************/
void LSPQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext, word16_t LSPCoefficients[], word16_t qLSPCoefficients[], uint16_t parameters[])
{
	int i,j;
	word16_t LSF[NB_LSP_COEFF]; /* LSF coefficients in Q2.13 range [0, Pi[ */
	word16_t weights[NB_LSP_COEFF]; /* weights in Q11 */
	word16_t weightsThreshold[NB_LSP_COEFF]; /* store in Q13 the threshold used to compute the weights */
	int L0;
	word32_t weightedMeanSquareError[L0_RANGE];
	word16_t L1index[L0_RANGE];
	word16_t L2index[L0_RANGE];
	word16_t L3index[L0_RANGE];
	word16_t quantizerOutput[NB_LSP_COEFF];
	word16_t qLSF[NB_LSP_COEFF];

	/*** compute LSF in Q2.13 : lsf = arcos(lsp) range [0, Pi[ spec 3.2.4 eq18 ***/
	for (i=0; i<NB_LSP_COEFF; i++)  {
		LSF[i] = g729Acos_Q15Q13(LSPCoefficients[i]);
	}

	/*** compute the weights vector as in spec 3.2.4 eq22 ***/
	weightsThreshold[0] = SUB16(LSF[1], OO4PIPLUS1_IN_Q13);
	for (i=1; i<NB_LSP_COEFF-1; i++) {
		weightsThreshold[i] = SUB16(SUB16(LSF[i+1], LSF[i-1]), ONE_IN_Q13);
		
	}
	weightsThreshold[NB_LSP_COEFF-1] = SUB16(O92PIMINUS1_IN_Q13, LSF[NB_LSP_COEFF-2]);

	for (i=0; i<NB_LSP_COEFF; i++) {
		if (weightsThreshold[i]>0) {
			weights[i] = ONE_IN_Q11;
		} else {
			weights[i] = (word16_t)SATURATE(ADD32(PSHR(MULT16_16(MULT16_16_Q13(weightsThreshold[i],  weightsThreshold[i]), 10), 2), ONE_IN_Q11), MAXINT16);
		}
	}
	weights[4] = MULT16_16_Q14(weights[4], ONE_POINT_2_IN_Q14);
	weights[5] = MULT16_16_Q14(weights[5], ONE_POINT_2_IN_Q14);

	/*** compute the coefficients for the two MA Predictors ***/
	for (L0=0; L0<L0_RANGE; L0++) {
		/* compute the target Vector (l) to be quantized as in spec 3.2.4 eq23 */
		word16_t targetVector[NB_LSP_COEFF]; /* vector to be quantized in Q13 */
		word32_t meanSquareDiff = MAXINT32;
		word16_t quantizedVector[NB_LSP_COEFF]; /* in Q13, the current state of quantized vector */

		for (i=0; i<NB_LSP_COEFF; i++) {
			word32_t acc = SHL(LSF[i],15); /* acc in Q2.28 */
			for (j=0; j<MA_MAX_K; j++) {
				acc = MSU16_16(acc, encoderChannelContext->previousqLSF[j][i], MAPredictor[L0][j][i]); /* previousqLSF in Q2.13 and MAPredictor in Q0.15-> acc in Q2.28 */
			}
			targetVector[i] = MULT16_16_Q12((word16_t)PSHR(acc, 15), invMAPredictorSum[L0][i]); /* acc->Q13 and invMAPredictorSum in Q12 -> targetVector in Q13 */
		}

		/* find closest match for predictionError (minimize mean square diff) in L1 codebook */
		for (i=0; i<L1_RANGE; i++) {
			word32_t acc = 0;
			for (j=0; j<NB_LSP_COEFF; j++) {
				word16_t difftargetVectorL1 = SATURATE(SUB32(targetVector[j], L1[i][j]), MAXINT16);
				acc = MAC16_16(acc, difftargetVectorL1, difftargetVectorL1);
			}

			if (acc<meanSquareDiff) {
				meanSquareDiff = acc;
				L1index[L0] = i;
			}
		}
		
		/* find the closest match in L2 wich will minimise the weighted sum of (targetVector - L1 result - L2)^2 */
		/* using eq20, eq21 and eq23 in spec 3.2.4 -> l[i] - l^[i] = (wi - w^[i])/(1-SumMAPred[i]) but ITU code ignores this denominator */
		/* works on the first five coefficients only */
		meanSquareDiff = MAXINT32;
		for (i=0; i<L2_RANGE; i++) {
			word32_t acc = 0;
			for (j=0; j<NB_LSP_COEFF/2; j++) {
				/* commented code : compute in the same way of the ITU code: ignore the denonimator and minimize (wi - w^[i])/(1-SumMAPred[i]) instead of (wi - w^[i]) square sum */
				//word16_t difftargetVectorL1L2 = SATURATE(SUB32(SUB32(targetVector[j], L1[L1index[L0]][j]), L2L3[i][j]), MAXINT16); /* targetVector, L1 and L2L3 in Q13 -> result in Q13 */
				word16_t difftargetVectorL1L2 = SATURATE(MULT16_16_Q15(SUB32(SUB32(targetVector[j], L1[L1index[L0]][j]), L2L3[i][j]), MAPredictorSum[L0][j]), MAXINT16); /* targetVector, L1 and L2L3 in Q13 -> result in Q13 */
				acc = MAC16_16(acc, difftargetVectorL1L2, MULT16_16_Q11(difftargetVectorL1L2, weights[j])); /* weights in Q11, diff in Q13 */
			}

			if (acc<meanSquareDiff) {
				meanSquareDiff = acc;
				L2index[L0] = i;
			}
		}

		/* find the closest match in L3 wich will minimise the weighted sum of (targetVector - L1 result - L3)^2 */
		/* using eq20, eq21 and eq23 in spec 3.2.4 -> l[i] - l^[i] = (wi - w^[i])/(1-SumMAPred[i]) but ITU code ignores this denominator */
		/* works on the first five coefficients only */
		meanSquareDiff = MAXINT32;
		for (i=0; i<L2_RANGE; i++) {
			word32_t acc = 0;
			for (j=NB_LSP_COEFF/2; j<NB_LSP_COEFF; j++) {
				/* commented code : compute in the same way of the ITU code: ignore the denonimator and minimize (wi - w^[i])/(1-SumMAPred[i]) instead of (wi - w^[i]) square sum */
				//word16_t difftargetVectorL1L3 = SATURATE(SUB32(SUB32(targetVector[j], L1[L1index[L0]][j]), L2L3[i][j]), MAXINT16); /* targetVector, L1 and L2L3 in Q13 -> result in Q13 */
				word16_t difftargetVectorL1L3 = SATURATE(MULT16_16_Q15(SUB32(SUB32(targetVector[j], L1[L1index[L0]][j]), L2L3[i][j]), MAPredictorSum[L0][j]), MAXINT16); /* targetVector, L1 and L2L3 in Q13 -> result in Q13 */
				acc = MAC16_16(acc, difftargetVectorL1L3, MULT16_16_Q11(difftargetVectorL1L3, weights[j])); /* weights in Q11, diff in Q13 */
			}

			if (acc<meanSquareDiff) {
				meanSquareDiff = acc;
				L3index[L0] = i;
			}
		}

		/* compute the quantized vector L1+L2/L3 and rearrange it as specified in spec 3.2.4(first the higher part (L2) and then the lower part (L3)) */
		/* Note: according to the spec, the rearrangement shall be done on each candidate while looking for best match, but the ITU code does it after picking the best match and so we do */
		for (i=0; i<NB_LSP_COEFF/2; i++) {
			quantizedVector[i] = ADD16(L1[L1index[L0]][i], L2L3[L2index[L0]][i]);
		}
		for (i=NB_LSP_COEFF/2; i<NB_LSP_COEFF; i++) {
			quantizedVector[i] = ADD16(L1[L1index[L0]][i], L2L3[L3index[L0]][i]);
		}

		/* rearrange with a minimum distance of 0.0012 */
		for (i=1; i<NB_LSP_COEFF/2; i++) {
			if (quantizedVector[i-1]>SUB16(quantizedVector[i],GAP1)) {
				quantizedVector[i-1] = PSHR(SUB16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
				quantizedVector[i] = PSHR(ADD16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
			}
		}
		for (i=NB_LSP_COEFF/2+1; i<NB_LSP_COEFF; i++) {
			if (quantizedVector[i-1]>SUB16(quantizedVector[i],GAP1)) {
				quantizedVector[i-1] = PSHR(SUB16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
				quantizedVector[i] = PSHR(ADD16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP1), 1);
			}
		}

		/* rearrange the whole quantizedVector with a distance of 0.0006 */
		for (i=1; i<NB_LSP_COEFF; i++) {
			if (quantizedVector[i-1]>SUB16(quantizedVector[i],GAP2)) {
				quantizedVector[i-1] = PSHR(SUB16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP2), 1);
				quantizedVector[i] = PSHR(ADD16(ADD16(quantizedVector[i], quantizedVector[i-1]), GAP2), 1);
			}
		}

		/* compute the weighted mean square distance using the final quantized vector according to eq21 */
		weightedMeanSquareError[L0]=0;
		for (i=0; i<NB_LSP_COEFF; i++) {
			word16_t difftargetVectorQuantizedVector = SATURATE(MULT16_16_Q15(SUB32(targetVector[i], quantizedVector[i]), MAPredictorSum[L0][i]), MAXINT16); /* targetVector and quantizedVector in Q13 -> result in Q13 */
			weightedMeanSquareError[L0] = MAC16_16(weightedMeanSquareError[L0], difftargetVectorQuantizedVector, MULT16_16_Q11(difftargetVectorQuantizedVector, weights[i])); /* weights in Q11, diff in Q13 */
		}
	}

	/* now select L0 and copy the selected coefficients to the output buffer */
	if (weightedMeanSquareError[0]<weightedMeanSquareError[1]) {
		parameters[0] = 0;
		parameters[1] = L1index[0];
		parameters[2] = L2index[0];
		parameters[3] = L3index[0];
	} else {
		parameters[0] = 1;
		parameters[1] = L1index[1];
		parameters[2] = L2index[1];
		parameters[3] = L3index[1];
	}

	/*** Compute the quantized LSF from the L coefficients ***/
	/* reconstruct vector from the codebooks using the selected parameters spec 3.2.4 eq19 */
	for (i=0; i<NB_LSP_COEFF/2; i++) {
		quantizerOutput[i] = ADD16(L1[parameters[1]][i], L2L3[parameters[2]][i]); /* codebooks are in Q2.13 for L1 and Q0.13 for L2L3, due to actual values stored in the codebooks, result in Q2.13 */
	}
	for ( i=NB_LSP_COEFF/2; i<NB_LSP_COEFF; i++) {
		quantizerOutput[i] = ADD16(L1[parameters[1]][i], L2L3[parameters[3]][i]); /* same as previous, output in Q2.13 */
	}
	/* rearrange in order to have a minimum distance between two consecutives coefficients spec 3.2.4 */
	rearrangeCoefficients(quantizerOutput, GAP1); 
	rearrangeCoefficients(quantizerOutput, GAP2); /* currentqLSF still in Q2.13 */

	/* compute qLSF spec 3.2.4 eq20 */
	for (i=0; i<NB_LSP_COEFF; i++) { 
		word32_t acc = MULT16_16(MAPredictorSum[parameters[0]][i], quantizerOutput[i]); /* (1 - ∑Pi,k)*lˆi(m) Q15 * Q13 -> Q28 */
		for (j=0; j<MA_MAX_K; j++) {
			acc = MAC16_16(acc, MAPredictor[parameters[0]][j][i], encoderChannelContext->previousqLSF[j][i]); 
		}
		/* acc in Q2.28, shift back the acc to a Q2.13 with rounding */
		qLSF[i] = (word16_t)PSHR(acc, 15); /* qLSF in Q2.13 */
	}

	/* update the previousqLSF buffer with current quantizer output */
	for (i=MA_MAX_K-1; i>0; i--) {
		memcpy(encoderChannelContext->previousqLSF[i], encoderChannelContext->previousqLSF[i-1], NB_LSP_COEFF*sizeof(word16_t));
	}
	memcpy(encoderChannelContext->previousqLSF[0], quantizerOutput, NB_LSP_COEFF*sizeof(word16_t));

	/*** qLSF stability check ***/
	insertionSort(qLSF, NB_LSP_COEFF);
	
	/* check for low limit on qLSF[0] */
	if (qLSF[1]<qLSF_MIN) {
		qLSF[1] = qLSF_MIN;
	}

	/* check and rectify minimum distance between two consecutive qLSF */
	for (i=0; i<NB_LSP_COEFF-1; i++) {
		if (SUB16(qLSF[i+1],qLSF[i])<MIN_qLSF_DISTANCE) {
			qLSF[i+1] = qLSF[i]+MIN_qLSF_DISTANCE;
		}
	}

	/* check for upper limit on qLSF[NB_LSP_COEFF-1] */
	if (qLSF[NB_LSP_COEFF-1]>qLSF_MAX) {
		qLSF[NB_LSP_COEFF-1] = qLSF_MAX;
	}

	/* convert qLSF to qLSP: qLSP = cos(qLSF) */
	for (i=0; i<NB_LSP_COEFF; i++) {
		qLSPCoefficients[i] = g729Cos_Q13Q15(qLSF[i]); /* ouput in Q0.15 */
	}

	return;
}
