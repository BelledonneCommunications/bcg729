/*
 dtx.c
 Comfort Noise Generation

 Copyright (C) 2015 Belledonne Communications, Grenoble, France
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

#include "dtx.h"
#include "computeLP.h"
#include "g729FixedPointMath.h"
#include "LP2LSPConversion.h"
#include "LSPQuantization.h"
#include "codebooks.h"
#include "cng.h"
#include "interpolateqLSP.h"
#include "qLSP2LP.h"

#define SID_FRAME 2
#define UNTRANSMITTED_FRAME 0
/*** local functions ***/
static void sumAutocorrelationCoefficients(word32_t autoCorrelationCoefficients[][NB_LSP_COEFF+1], int8_t *autocorrelationCoefficientsScale, uint8_t nbElements,
		word32_t *autoCorrelationCoefficientsResult, int8_t *autocorrelationCoefficientsScaleResults) {
	
	int i,j;
	word64_t autoCorrelationSumBuffer[NB_LSP_COEFF+1]; /* used to temporary store sum on 64 bits */
	word64_t max=0;
	word32_t rescaledAutocorrelationCoefficients[7][NB_LSP_COEFF+1]; /* used to temporary stored rescaled elements */
	int8_t rightShiftToNormalise = 0;

	/* get lowest scale */
	int8_t minScale = autocorrelationCoefficientsScale[0];
	for (i=1; i<nbElements; i++) {
		if (autocorrelationCoefficientsScale[i]<minScale) {
			minScale=autocorrelationCoefficientsScale[i];
		}
	}

	/* rescale the coefficients */
	for (j=0; j<nbElements; j++) {
		int8_t rescaling = autocorrelationCoefficientsScale[j] - minScale;
		for (i=0; i<NB_LSP_COEFF+1; i++) {
			rescaledAutocorrelationCoefficients[j][i] = SHR32(autoCorrelationCoefficients[j][i], rescaling);
		}
	}

	/* sum them on 64 bits and get the maximum value reached */
	for (i=0; i<NB_LSP_COEFF+1; i++) {
		autoCorrelationSumBuffer[i] = rescaledAutocorrelationCoefficients[0][i];
		for (j=1; j<nbElements; j++) {
			autoCorrelationSumBuffer[i] = ADD64(autoCorrelationSumBuffer[i], rescaledAutocorrelationCoefficients[j][i]);
		}
		if (ABS(autoCorrelationSumBuffer[i])>max) max = ABS(autoCorrelationSumBuffer[i]);
	}

	/* normalise result on 32 bits */
	if (max>MAXINT32) {
		do {
			max = SHR(max,1);
			rightShiftToNormalise++;
		} while (max>MAXINT32);
		
		for (i=0; i<NB_LSP_COEFF+1; i++) {
			autoCorrelationCoefficientsResult[i] = (word32_t)SHR64(autoCorrelationSumBuffer[i], rightShiftToNormalise);
		}
	} else {
		for (i=0; i<NB_LSP_COEFF+1; i++) {
			autoCorrelationCoefficientsResult[i] = (word32_t)(autoCorrelationSumBuffer[i]);
		}

	}

	/* adjust output scale according to possible right shift */
	*autocorrelationCoefficientsScaleResults = minScale - rightShiftToNormalise;

	return;
}

/*****************************************************************************/
/* residual Energy quantization according to spec B4.2.1                     */
/*   parameters:                                                             */
/*    -(i) residualEnergy : to be quantized in variable scale                */
/*    -(i) residualEnergyScale : scale of previous parameter                 */
/*    -(o) decodedLogEnergy : decode the quantized energy into a 10Log(E)    */
/*   returns the quantized residual energy parameterR(on 5 bits)             */
/*                                                                           */
/*****************************************************************************/
static uint8_t residualEnergyQuantization(word32_t residualEnergy, int8_t residualEnergyScale, int8_t *decodedLogEnergy) {
	word32_t acc;
	acc = SUB32(g729Log2_Q0Q16(residualEnergy), ADD32(479849, (int32_t)(residualEnergyScale<<16))); /* -479849 is log2(aw/(NCur*80)) acc is log2(E') in Q16 aw = 1 as we use unlagged  autocorrelation coefficients */
	acc = SHR32(acc,1); /* acc = log2(E') in Q15 */
	acc = MULT16_32_Q15(INV_LOG2_10_Q15, acc); /* acc log10(E') in Q15 */

	/* quantization acc contains value to be quantized/10 so all constant / 10 respect what is in the spec */
	if (acc < - 26214) { /* -0.8 in Q15 */
		*decodedLogEnergy = -12;
		return 0; /* first step by 8 */
	} else if (acc < 45875 ) { /* 1,4 in Q15 */
		uint8_t steps;
		acc = (acc+19661); /* up to 14, step by 0.4*/
		if (acc<0) {
			acc= 0;
		} else {
			acc = MULT16_32_Q13(20480, acc); /* step by 0.4, 20480 is 1/0.4 in Q13 -> acc still in Q15 */
		}
		steps = SHR(acc,15);
		*decodedLogEnergy = -2 +4*steps;
		return 1+steps; 
	} else  if (acc<216268) { /* check that log(E') is not > 66dB (216268 is 6.6 in Q15) */
		uint8_t steps;
		acc = (acc-49152); /* -1.5 in Q15 */
		if (acc<0) {
			acc = 0;
		} else {
			acc = MULT16_32_Q12(20480, acc); /* step by 0.2, 20480 is 1/0.2 in Q12 -> acc still in Q15 */
		}
		steps = SHR(acc,15);
		*decodedLogEnergy = 16 +2*steps;
		return 6+steps;
	} else { /* quantized energy support up to 66dB */
		*decodedLogEnergy = 66;
		return 31;
	}
}

/*****************************************************************************/
/* compute LP Coefficients auto correlation as in eq B.13                    */
/*   parameters:                                                             */
/*    -(i) LPCoefficients : in Q12, 10 values, 1 LP Coeff is always 1 and not*/
/*                          stored in this buffer                            */
/*    -(o) LPCautocorrelation : 11 values in Q20                             */
/*                                                                           */
/*****************************************************************************/
static void computeLPCoefficientAutocorrelation(word16_t *LPCoefficients, word32_t *LPCautocorrelation) {
	int j,k;
	/* first compute Ra0 */
	LPCautocorrelation[0] = 4096*4096>>4; /* LPCoefficients are in Q12 -> acc in Q24, init: acc = LP(0)*LP(0) -> 1 in Q20 as LP(0) is not stored because always 1 */
	for (k=0; k<NB_LSP_COEFF; k++) {
		LPCautocorrelation[0] = MAC16_16_Q4(LPCautocorrelation[0], LPCoefficients[k], LPCoefficients[k]); /* LPCoefficients in Q12*Q12 -> Q24 >> Q4: result in Q20 */
	}

	/* and the rest */
	for (j=1; j<NB_LSP_COEFF+1; j++) {
		LPCautocorrelation[j] = SHL(LPCoefficients[j-1],9); /* LPCoeff[0] is not stored always 1, so LPCoeff index is -1 respect LPCautocorrelation, SHL(9) to make *2 and get it in Q20 from Q12 */
		for (k=0; k<10-j; k++) {
			LPCautocorrelation[j] = MAC16_16_Q3(LPCautocorrelation[j], LPCoefficients[k], LPCoefficients[k+j]); /* this k is actually k-1 respect to eq B.13 Q12*Q12 -> Q24 >> 3 : *2 and result in Q20 */
		}

	}
}


/********************************************************************************/
/* compare LPC filters: as in spec B.4.1.3 eq B.12                              */
/*   parameters:                                                                */
/*    -(i) LPCoefficientsAutocorrelation: 11 values in Q20, Ra in spec          */
/*    -(i) autocorrelationCoefficients: 11 values in variable scale, Rt in spec */
/*    -(i) residualEnergy : in the same scale as previous value, Et in spec     */
/*    -(i) threshold : in Q20                                                   */
/*   return 1 if the filter significantly differs (eq B.12 is true)             */
/*                                                                              */
/********************************************************************************/
static uint8_t compareLPCFilters(word32_t *LPCoefficientsAutocorrelation, word32_t *autocorrelationCoefficients, word32_t residualEnergy, word32_t threshold) {
	/* on the left term of the comparison we have Ra in Q20 an Rt in variable scale */
	/* on the right term of the comparison we have Threshold in Q20 an Et in variable scale but same as Rt */
	word64_t acc = 0;
	int i;
	for (i=0; i<NB_LSP_COEFF+1; i++) {
		acc = MAC64(acc, LPCoefficientsAutocorrelation[i], autocorrelationCoefficients[i]);
	}

	if (acc >= MULT32_32(residualEnergy, threshold)) {
		return 1;
	} else {
		return 0;
	}
}
/*****************************************************************************/
/* initBcg729DTXChannel : create context structure and initialise it         */
/*    return value :                                                         */
/*      - the DTX channel context data                                       */
/*                                                                           */
/*****************************************************************************/
bcg729DTXChannelContextStruct *initBcg729DTXChannel() {
	int i;
	/* create the context structure */
	bcg729DTXChannelContextStruct *DTXChannelContext = malloc(sizeof(bcg729DTXChannelContextStruct));
	memset(DTXChannelContext, 0, sizeof(*DTXChannelContext)); /* set autocorrelation buffers to 0 */
	/* avoid arithmetics problem: set past autocorrelation[0] to 1 */
	for (i=0; i<7; i++) {
		DTXChannelContext->autocorrelationCoefficients[i][0] = ONE_IN_Q30;
		DTXChannelContext->autocorrelationCoefficientsScale[i] = 30;
	}

	DTXChannelContext->previousVADflag = 1; /* previous VAD flag must be initialised to VOICE */
	DTXChannelContext->pseudoRandomSeed = CNG_DTX_RANDOM_SEED_INIT;
	return DTXChannelContext;
}

/*******************************************************************************************/
/* updateDTXContext : save autocorrelation value in DTX context as requested in B4.1.1     */
/*   parameters:                                                                           */
/*    -(i/o) DTXChannelContext : the DTX context to be updated                             */
/*    -(i) autocorrelationsCoefficients : 11 values of variable scale, values are copied   */
/*          in DTX context                                                                 */
/*    -(i) autocorrelationCoefficientsScale : the scale of previous buffer(can be <0)      */
/*                                                                                         */
/*******************************************************************************************/
void updateDTXContext(bcg729DTXChannelContextStruct *DTXChannelContext, word32_t *autocorrelationCoefficients, int8_t autocorrelationCoefficientsScale) {
	/* move previous autocorrelation coefficients and store the new one */ 
	/* TODO: optimise it buy using rolling index */
	memcpy(DTXChannelContext->autocorrelationCoefficients[6], DTXChannelContext->autocorrelationCoefficients[5], (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[6] = DTXChannelContext->autocorrelationCoefficientsScale[5];
	memcpy(DTXChannelContext->autocorrelationCoefficients[5], DTXChannelContext->autocorrelationCoefficients[4], (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[5] = DTXChannelContext->autocorrelationCoefficientsScale[4];
	memcpy(DTXChannelContext->autocorrelationCoefficients[4], DTXChannelContext->autocorrelationCoefficients[3], (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[4] = DTXChannelContext->autocorrelationCoefficientsScale[3];
	memcpy(DTXChannelContext->autocorrelationCoefficients[3], DTXChannelContext->autocorrelationCoefficients[2], (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[3] = DTXChannelContext->autocorrelationCoefficientsScale[2];
	memcpy(DTXChannelContext->autocorrelationCoefficients[2], DTXChannelContext->autocorrelationCoefficients[1], (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[2] = DTXChannelContext->autocorrelationCoefficientsScale[1];
	memcpy(DTXChannelContext->autocorrelationCoefficients[1], DTXChannelContext->autocorrelationCoefficients[0], (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[1] = DTXChannelContext->autocorrelationCoefficientsScale[0];
	memcpy(DTXChannelContext->autocorrelationCoefficients[0], autocorrelationCoefficients, (NB_LSP_COEFF+1)*sizeof(word32_t));
	DTXChannelContext->autocorrelationCoefficientsScale[0] = autocorrelationCoefficientsScale;
}

/*******************************************************************************************/
/* encodeSIDFrame: called at eache frame even if VADflag is set to active speech           */
/*   Update the previousVADflag and if curent is set to NOISE, compute the SID params      */
/*  parameters:                                                                            */
/*   -(i/o) DTXChannelContext: current DTX context, is updated by this function            */
/*   -(o)   previousLSPCoefficients : 10 values in Q15, is updated by this function        */
/*   -(i/o) previousqLSPCoefficients : 10 values in Q15, is updated by this function       */
/*   -(i) VADflag : 1 active voice frame, 0 noise frame                                    */
/*   -(i/o) previousqLSF : set of 4 last frames qLSF in Q2.13, is updated                  */
/*   -(i/o) excicationVector : in Q0, accessed in range [-L_PAST_EXCITATION,L_FRAME-1]     */
/*   -(o) qLPCoefficients : 20 values in Q3.12  the quantized LP coefficients              */
/*   -(o) bitStream : SID frame parameters on 2 bytes, may be null if no frame is to be    */
/*        transmitted                                                                      */
/*   -(o) bitStreamLength : length of bitStream buffer to be transmitted (2 for SID, 0 for */
/*        untransmitted frame)                                                             */
/*                                                                                         */
/*******************************************************************************************/
void encodeSIDFrame(bcg729DTXChannelContextStruct *DTXChannelContext, word16_t *previousLSPCoefficients, word16_t *previousqLSPCoefficients,  uint8_t VADflag, word16_t previousqLSF[MA_MAX_K][NB_LSP_COEFF], word16_t *excitationVector, word16_t *qLPCoefficients, uint8_t *bitStream, uint8_t *bitStreamLength) {

	int i;
	word32_t summedAutocorrelationCoefficients[NB_LSP_COEFF+1];
	word16_t LPCoefficients[NB_LSP_COEFF]; /* in Q12 */
	word16_t LSPCoefficients[NB_LSP_COEFF]; /* in Q15 */
	word32_t reflectionCoefficients[NB_LSP_COEFF]; /* product of LP Computation, may be used if we need to generate the RFC3389 payload */
	word32_t residualEnergy; /* in variable scale(summedAutocorrelationCoefficientsScale) computed together with LP coefficients */
	int8_t summedAutocorrelationCoefficientsScale;
	uint8_t frameType;
	word32_t meanEnergy;
	int8_t meanEnergyScale;
	uint8_t quantizedResidualEnergy;
	int8_t decodedLogEnergy;
	uint8_t parameters[3]; /* array of the first 3 output parameters, 4th is in quantizedResidualEnergy */
	word16_t interpolatedqLSP[NB_LSP_COEFF]; /* the interpolated qLSP used for first subframe in Q15 */


	if (VADflag == 1) {/* this is a voice frame, just update the VADflag history and return */
		DTXChannelContext->pseudoRandomSeed = CNG_DTX_RANDOM_SEED_INIT; /* re-init pseudo random seed at each active frame to keep CNG and DTX in sync */
		DTXChannelContext->previousVADflag = 1;
		return;
	}

	/* NOISE frame */
	/* compute the autocorrelation coefficients sum on the current and previous frame */
	sumAutocorrelationCoefficients((DTXChannelContext->autocorrelationCoefficients), DTXChannelContext->autocorrelationCoefficientsScale, 2,
		summedAutocorrelationCoefficients, &summedAutocorrelationCoefficientsScale);

  	/* compute LP filter coefficients */
	autoCorrelation2LP(summedAutocorrelationCoefficients, LPCoefficients, reflectionCoefficients, &residualEnergy); /* output residualEnergy with the same scale of input summedAutocorrelationCoefficients */

	/* determine type of frame SID or untrasmitted */
	if (DTXChannelContext->previousVADflag == 1) { /* if previous frame was active : we must generate a SID frame spec B.10 */
		frameType = SID_FRAME;
		meanEnergy = residualEnergy;
		meanEnergyScale = summedAutocorrelationCoefficientsScale;
		quantizedResidualEnergy = residualEnergyQuantization(meanEnergy, meanEnergyScale, &decodedLogEnergy);
	} else { /* previous frame was already non active, check if we have to generate a new SID frame according to spec 4.1.2-4.1.4 */
		int8_t flag_chang = 0;

		/* update meanEnergy using current and previous frame Energy : meanE = current+previous/2 : rescale both of them dividing by 2 and sum */
		/* eqB14 doesn't divide by KE but it's done in eqB15, do it now */
		if (summedAutocorrelationCoefficientsScale<DTXChannelContext->previousResidualEnergyScale) {
			meanEnergyScale = summedAutocorrelationCoefficientsScale;
			meanEnergy = ADD32(SHR(residualEnergy,1), VSHR32(DTXChannelContext->previousResidualEnergy, DTXChannelContext->previousResidualEnergyScale - summedAutocorrelationCoefficientsScale + 1));

		} else {
			meanEnergyScale = DTXChannelContext->previousResidualEnergyScale;
			meanEnergy = ADD32(VSHR32(residualEnergy,summedAutocorrelationCoefficientsScale - DTXChannelContext->previousResidualEnergyScale + 1), SHR(DTXChannelContext->previousResidualEnergy, 1));
		}
		quantizedResidualEnergy = residualEnergyQuantization(meanEnergy, meanEnergyScale, &decodedLogEnergy);

		/* comparison of LPC filters B4.1.3 : DTXChannelContext->SIDLPCoefficientAutocorrelation contains the last used filter LP coeffecients autocorrelation in Q20 */
		if (compareLPCFilters(DTXChannelContext->SIDLPCoefficientAutocorrelation, summedAutocorrelationCoefficients, residualEnergy, THRESHOLD1_IN_Q20) != 0) {
			flag_chang = 1;
		}

		/* comparison of the energies B4.1.4 */
		if (ABS(DTXChannelContext->previousDecodedLogEnergy - decodedLogEnergy)>2) {
			flag_chang = 1;
		}

		/* check if we have to transmit a SID frame eq B.11 */
		DTXChannelContext->count_fr++;
		if (DTXChannelContext->count_fr<3) { /* min 3 frames between 2 consecutive SID frames */
			frameType = UNTRANSMITTED_FRAME;
		} else {
			if (flag_chang == 1) {
				frameType = SID_FRAME;
			} else {
				frameType = UNTRANSMITTED_FRAME;
			}
			DTXChannelContext->count_fr = 3; /* counter on 8 bits, keep value low, we just need to know if it is > 3 */
		}

	}

	/* generate the SID frame */
	if (frameType == SID_FRAME) {
		word32_t SIDLPCAutocorrelationCoefficients[NB_LSP_COEFF+1];
		int8_t SIDLPCAutocorrelationCoefficientsScale;
		word16_t pastAverageLPCoefficients[NB_LSP_COEFF]; /* in Q12 */
		word32_t pastAverageReflectionCoefficients[NB_LSP_COEFF]; /* produced by LP computation, may be used if we have to generate RFC3389 payload */
		word32_t pastAverageResidualEnergy; /* not used here, by-product of LP coefficients computation */

		/* reset frame count */
		DTXChannelContext->count_fr = 0;

		/*** compute the past average filter on the last 6 past frames ***/
		sumAutocorrelationCoefficients(&(DTXChannelContext->autocorrelationCoefficients[1]), &(DTXChannelContext->autocorrelationCoefficientsScale[1]), 6,
			SIDLPCAutocorrelationCoefficients, &SIDLPCAutocorrelationCoefficientsScale);

	  	/* compute past average LP filter coefficients Ap in B4.2.2 */
		autoCorrelation2LP(SIDLPCAutocorrelationCoefficients, pastAverageLPCoefficients, pastAverageReflectionCoefficients, &pastAverageResidualEnergy); /* output residualEnergy with the same scale of input summedAutocorrelationCoefficients */
	
		/* select coefficients according to eq B.17 we have Ap in SIDLPCoefficients and At in LPCoefficients, store result, in Q12 in SIDLPCoefficients */
		/* check distance beetwen currently used filter and past filter : compute LPCoefficentAutocorrelation for the past average filter */
		computeLPCoefficientAutocorrelation(pastAverageLPCoefficients, DTXChannelContext->SIDLPCoefficientAutocorrelation);

		DTXChannelContext->decodedLogEnergy = decodedLogEnergy; /* store frame mean energy for RFC3389 payload generation */
		
		if (compareLPCFilters(DTXChannelContext->SIDLPCoefficientAutocorrelation, summedAutocorrelationCoefficients, residualEnergy, THRESHOLD3_IN_Q20) == 0) { /* use the past average filter */
			/* generate LSP coefficient using the past LP coefficients */
			if (!LP2LSPConversion(pastAverageLPCoefficients, LSPCoefficients)) {
				/* unable to find the 10 roots repeat previous LSP */
				memcpy(LSPCoefficients, previousqLSPCoefficients, NB_LSP_COEFF*sizeof(word16_t));
			}
			/* LPCoefficientAutocorrelation are already in DTXChannelContext */ 
			/* save the reflection coefficients in the DTX context as they will be requested to generate RFC3389 payload */
			memcpy(DTXChannelContext->reflectionCoefficients, pastAverageReflectionCoefficients, NB_LSP_COEFF*sizeof(word32_t));
		} else { /* use filter computed on current and previous frame */
			/* compute the LPCoefficientAutocorrelation for this filter and store them in DTXChannel */
			computeLPCoefficientAutocorrelation(LPCoefficients, DTXChannelContext->SIDLPCoefficientAutocorrelation);
			/* generate LSP coefficient current LP coefficients */
			if (!LP2LSPConversion(LPCoefficients, LSPCoefficients)) {
				/* unable to find the 10 roots repeat previous LSP */
				memcpy(LSPCoefficients, previousqLSPCoefficients, NB_LSP_COEFF*sizeof(word16_t));
			}
			/* save the reflection coefficients in the DTX context as they will be requested to generate RFC3389 payload */
			memcpy(DTXChannelContext->reflectionCoefficients, reflectionCoefficients, NB_LSP_COEFF*sizeof(word32_t));
		}

		/* update previousLSP coefficient buffer */
		memcpy(previousLSPCoefficients, LSPCoefficients, NB_LSP_COEFF*sizeof(word16_t));
	
		/* LSP quantization */	
		noiseLSPQuantization(previousqLSF, LSPCoefficients, DTXChannelContext->qLSPCoefficients, parameters);

		/* update previousDecodedLogEnergy and SIDGain */
		DTXChannelContext->previousDecodedLogEnergy = decodedLogEnergy;
		DTXChannelContext->currentSIDGain = SIDGainCodebook[quantizedResidualEnergy];
	}

	/* save current Frame Energy */
	DTXChannelContext->previousResidualEnergy = residualEnergy;
	DTXChannelContext->previousResidualEnergyScale = summedAutocorrelationCoefficientsScale;

	/* apply target gain smoothing eq B.19 */
	if(DTXChannelContext->previousVADflag == 1) {
		DTXChannelContext->smoothedSIDGain = DTXChannelContext->currentSIDGain;
	} else {
		DTXChannelContext->smoothedSIDGain = SUB16(DTXChannelContext->smoothedSIDGain, (DTXChannelContext->smoothedSIDGain>>3));
		DTXChannelContext->smoothedSIDGain = ADD16(DTXChannelContext->smoothedSIDGain, (DTXChannelContext->currentSIDGain>>3));
	}

	/* update excitation vector */
	computeComfortNoiseExcitationVector(DTXChannelContext->smoothedSIDGain, &(DTXChannelContext->pseudoRandomSeed), excitationVector);

	/* Interpolate qLSP and update the previousqLSP buffer */
	interpolateqLSP(previousqLSPCoefficients, DTXChannelContext->qLSPCoefficients, interpolatedqLSP); /* in case of untransmitted frame, use qLSP generated for the last transmitted one */
	for (i=0; i<NB_LSP_COEFF; i++) {
		previousqLSPCoefficients[i] = DTXChannelContext->qLSPCoefficients[i];
	}

	/* first subframe */
	qLSP2LP(interpolatedqLSP, qLPCoefficients);
	/* second subframe */
	qLSP2LP(DTXChannelContext->qLSPCoefficients, &(qLPCoefficients[NB_LSP_COEFF]));
	
	/* set parameters into the bitStream if a frame must be transmitted */
	if (frameType == SID_FRAME) {
		*bitStreamLength = 2;
		bitStream[0] = (((parameters[0]&0x01)<<7)  /* L0 1 bit */
			| ((parameters[1]&0x1F)<<2) /* L1 5 bits */
			| ((parameters[2]>>2)%0x03)); /* L2 is 4 bits 2 MSB in this byte */
		bitStream[1] = (((parameters[2]&0x03)<<6) /* 2 LSB of 4 bits L2 */
			| ((quantizedResidualEnergy&0x1F)<<1)); /* Gain 5 bits, last bit is left to 0 */
	} else {
		*bitStreamLength = 0;
	}

	/* update the previousVADflag in context */
	DTXChannelContext->previousVADflag = 0;

}
