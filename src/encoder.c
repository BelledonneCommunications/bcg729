/*
 encoder.c

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

#include "bcg729/encoder.h"

#include "interpolateqLSP.h"
#include "qLSP2LP.h"

#include "preProcessing.h"
#include "computeLP.h"
#include "LP2LSPConversion.h"
#include "LSPQuantization.h"
#include "computeWeightedSpeech.h"
#include "findOpenLoopPitchDelay.h"
#include "adaptativeCodebookSearch.h"
#include "computeAdaptativeCodebookGain.h"
#include "fixedCodebookSearch.h"
#include "gainQuantization.h"
#include "g729FixedPointMath.h"
#include "vad.h"
#include "dtx.h"

/* buffers allocation */
static const word16_t previousLSPInitialValues[NB_LSP_COEFF] = {30000, 26000, 21000, 15000, 8000, 0, -8000,-15000,-21000,-26000}; /* in Q0.15 the initials values for the previous LSP buffer */

/*****************************************************************************/
/* initBcg729EncoderChannel : create context structure and initialise it     */
/*    return value :                                                         */
/*      - the encoder channel context data                                   */
/*                                                                           */
/*****************************************************************************/
bcg729EncoderChannelContextStruct *initBcg729EncoderChannel(uint8_t enableVAD)
{
	/* create the context structure */
	bcg729EncoderChannelContextStruct *encoderChannelContext = malloc(sizeof(bcg729EncoderChannelContextStruct));
	memset(encoderChannelContext, 0, sizeof(bcg729EncoderChannelContextStruct));

	/* initialise statics buffers and variables */
	memset(encoderChannelContext->signalBuffer, 0, (L_LP_ANALYSIS_WINDOW-L_FRAME)*sizeof(word16_t)); /* set to zero all the past signal */
	encoderChannelContext->signalLastInputFrame = &(encoderChannelContext->signalBuffer[L_LP_ANALYSIS_WINDOW-L_FRAME]); /* point to the last frame in the signal buffer */
	encoderChannelContext->signalCurrentFrame = &(encoderChannelContext->signalBuffer[L_LP_ANALYSIS_WINDOW-L_SUBFRAME-L_FRAME]); /* point to the current frame */
	memcpy(encoderChannelContext->previousLSPCoefficients, previousLSPInitialValues, NB_LSP_COEFF*sizeof(word16_t)); /* reset the previous quantized and unquantized LSP vector with the same value */
	memcpy(encoderChannelContext->previousqLSPCoefficients, previousLSPInitialValues, NB_LSP_COEFF*sizeof(word16_t));
	memset(encoderChannelContext->weightedInputSignal, 0, MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t)); /* set to zero values of previous weighted signal */
	memset(encoderChannelContext->excitationVector, 0, L_PAST_EXCITATION*sizeof(word16_t)); /* set to zero values of previous excitation vector */
	memset(encoderChannelContext->targetSignal, 0, NB_LSP_COEFF*sizeof(word16_t)); /* set to zero values filter memory for the targetSignal computation */
	encoderChannelContext->lastQuantizedAdaptativeCodebookGain = O2_IN_Q14; /* quantized gain is initialized at his minimum value: 0.2 */
	if (enableVAD == 1) {
		encoderChannelContext->VADChannelContext = initBcg729VADChannel();
		encoderChannelContext->DTXChannelContext = initBcg729DTXChannel();
	} else {
		encoderChannelContext->VADChannelContext = NULL;
		encoderChannelContext->DTXChannelContext = NULL;
	}

	/* initialisation of the differents blocs which need to be initialised */
	initPreProcessing(encoderChannelContext);
	initLSPQuantization(encoderChannelContext);
	initGainQuantization(encoderChannelContext);

	return encoderChannelContext;
}

/*****************************************************************************/
/* closeBcg729EncoderChannel : free memory of context structure              */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : the channel context data                */
/*                                                                           */
/*****************************************************************************/
void closeBcg729EncoderChannel(bcg729EncoderChannelContextStruct *encoderChannelContext)
{
	if (encoderChannelContext) {
		if (encoderChannelContext->VADChannelContext) {
			free(encoderChannelContext->VADChannelContext);
		}
		if (encoderChannelContext->DTXChannelContext) {
			free(encoderChannelContext->DTXChannelContext);
		}
		free(encoderChannelContext);
	}
}

/*****************************************************************************/
/* bcg729Encoder :                                                           */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : context for this encoder channel        */
/*      -(i) inputFrame : 80 samples (16 bits PCM)                           */
/*      -(o) bitStream : The 15 parameters for a frame on 80 bits            */
/*           on 80 bits (10 8bits words)                                     */
/*                                                                           */
/*****************************************************************************/
void bcg729Encoder(bcg729EncoderChannelContextStruct *encoderChannelContext, int16_t inputFrame[], uint8_t bitStream[], uint8_t *bitStreamLength)
{
	int i;
	uint16_t parameters[NB_PARAMETERS]; /* the output parameters in an array */

	/* internal buffers which we do not need to keep between calls */
	word16_t LPCoefficients[NB_LSP_COEFF]; /* the LP coefficients in Q3.12 */
	word16_t LSFCoefficients[NB_LSP_COEFF]; /* the LSF coefficients in Q3.12 */
	word16_t qLPCoefficients[2*NB_LSP_COEFF]; /* the quantized LP coefficients in Q3.12 computed from the qLSP one after interpolation: two sets, one for each subframe */
	word16_t weightedqLPCoefficients[2*NB_LSP_COEFF]; /* the qLP coefficients in Q3.12 weighted according to spec A3.3.3 */
	word16_t LSPCoefficients[NB_LSP_COEFF]; /* the LSP coefficients in Q15 */
	word16_t qLSPCoefficients[NB_LSP_COEFF]; /* the quantized LSP coefficients in Q15 */
	word16_t interpolatedqLSP[NB_LSP_COEFF]; /* the interpolated qLSP used for first subframe in Q15 */

	uint16_t openLoopPitchDelay;
	int16_t intPitchDelayMin;
	int16_t intPitchDelayMax;
	int subframeIndex;
	int LPCoefficientsIndex = 0;
	int parametersIndex = 4; /* index to insert parameters in the parameters output array */
	word16_t impulseResponseInput[L_SUBFRAME]; /* input buffer for the impulse response computation: in Q12, 1 followed by all zeros see spec A3.5*/

	/* used for VAD */
	word32_t reflectionCoefficients[NB_LSP_COEFF]; /* in Q31, computed during LP generation, reflectionCoefficients[1] is requested by VAD, the others are stored in context if we cover RFC3389 payload generation */
	word32_t autoCorrelationCoefficients[NB_LSP_COEFF+3]; /* if VAD is enabled we must compute 13 coefficients, 11 otherwise but used only internally by computeLP function in that case */
	word32_t noLagAutoCorrelationCoefficients[NB_LSP_COEFF+3]; /* DTX must have access to autocorrelation Coefficients on which lag windowing as not been applied */
	int8_t autoCorrelationCoefficientsScale; /* autocorrelation coefficients are normalised by computeLP, must get their scaling factor */

	/*****************************************************************************************/
	/*** on frame basis : preProcessing, LP Analysis, Open-loop pitch search               ***/
	preProcessing(encoderChannelContext, inputFrame, encoderChannelContext->signalLastInputFrame); /* output of the function in the signal buffer */

	/* use the whole signal Buffer for windowing and autocorrelation */
	/* autoCorrelation Coefficients are computed and used internally, in case of VAD we must compute and retrieve 13 coefficients, compute only 11 when VAD is disabled */
	computeLP(encoderChannelContext->signalBuffer, LPCoefficients, reflectionCoefficients, autoCorrelationCoefficients, noLagAutoCorrelationCoefficients, &autoCorrelationCoefficientsScale, (encoderChannelContext->VADChannelContext != NULL)?(NB_LSP_COEFF+3):(NB_LSP_COEFF+1));
	/*** compute LSP: it might fail, get the previous one in this case ***/
	if (!LP2LSPConversion(LPCoefficients, LSPCoefficients)) {
		/* unable to find the 10 roots repeat previous LSP */
		memcpy(LSPCoefficients, encoderChannelContext->previousLSPCoefficients, NB_LSP_COEFF*sizeof(word16_t));
	}

	/*********** VAD *****************/
	if (encoderChannelContext->VADChannelContext != NULL) { /* if VAD is not enable, no context */
		uint8_t VADflag = 1;
		/* update DTX context */
		updateDTXContext(encoderChannelContext->DTXChannelContext, noLagAutoCorrelationCoefficients, autoCorrelationCoefficientsScale);

		/*** compute LSF in Q2.13 : lsf = arcos(lsp) range [0, Pi[ spec 3.2.4 eq18 ***/
		/* TODO : remove it from LSPQuantizationFunction and perform it out of enableVAD test */
		for (i=0; i<NB_LSP_COEFF; i++)  {
			LSFCoefficients[i] = g729Acos_Q15Q13(LSPCoefficients[i]);
		}

		VADflag = bcg729_vad(encoderChannelContext->VADChannelContext, reflectionCoefficients[1], LSFCoefficients, autoCorrelationCoefficients, autoCorrelationCoefficientsScale, encoderChannelContext->signalCurrentFrame);

		/* call encodeSIDFrame even if it is a voice frame as it will update DTXContext with current VADflag : TODO : move updateDTXContext in the encodeSIDFrame as part of the update is performed in it anyway */
		encodeSIDFrame(encoderChannelContext->DTXChannelContext,  encoderChannelContext->previousLSPCoefficients, encoderChannelContext->previousqLSPCoefficients, VADflag, encoderChannelContext->previousqLSF, &(encoderChannelContext->excitationVector[L_PAST_EXCITATION]), qLPCoefficients, bitStream, bitStreamLength);

		if (VADflag == 0 ) { /* NOISE frame has been encoded */
			word16_t residualSignal[L_FRAME];
			/* update encoder context : generate weighted signal */
			/*** Compute the weighted Quantized LP Coefficients according to spec A3.3.3 ***/
			/*  weightedqLPCoefficients[0] = qLPCoefficients[0]*Gamma^(i+1) (i=0..9) with Gamma = 0.75 in Q15 */
			weightedqLPCoefficients[0] = MULT16_16_P15(qLPCoefficients[0], GAMMA_E1);
			weightedqLPCoefficients[1] = MULT16_16_P15(qLPCoefficients[1], GAMMA_E2);
			weightedqLPCoefficients[2] = MULT16_16_P15(qLPCoefficients[2], GAMMA_E3);
			weightedqLPCoefficients[3] = MULT16_16_P15(qLPCoefficients[3], GAMMA_E4);
			weightedqLPCoefficients[4] = MULT16_16_P15(qLPCoefficients[4], GAMMA_E5);
			weightedqLPCoefficients[5] = MULT16_16_P15(qLPCoefficients[5], GAMMA_E6);
			weightedqLPCoefficients[6] = MULT16_16_P15(qLPCoefficients[6], GAMMA_E7);
			weightedqLPCoefficients[7] = MULT16_16_P15(qLPCoefficients[7], GAMMA_E8);
			weightedqLPCoefficients[8] = MULT16_16_P15(qLPCoefficients[8], GAMMA_E9);
			weightedqLPCoefficients[9] = MULT16_16_P15(qLPCoefficients[9], GAMMA_E10);
			weightedqLPCoefficients[10] = MULT16_16_P15(qLPCoefficients[10], GAMMA_E1);
			weightedqLPCoefficients[11] = MULT16_16_P15(qLPCoefficients[11], GAMMA_E2);
			weightedqLPCoefficients[12] = MULT16_16_P15(qLPCoefficients[12], GAMMA_E3);
			weightedqLPCoefficients[13] = MULT16_16_P15(qLPCoefficients[13], GAMMA_E4);
			weightedqLPCoefficients[14] = MULT16_16_P15(qLPCoefficients[14], GAMMA_E5);
			weightedqLPCoefficients[15] = MULT16_16_P15(qLPCoefficients[15], GAMMA_E6);
			weightedqLPCoefficients[16] = MULT16_16_P15(qLPCoefficients[16], GAMMA_E7);
			weightedqLPCoefficients[17] = MULT16_16_P15(qLPCoefficients[17], GAMMA_E8);
			weightedqLPCoefficients[18] = MULT16_16_P15(qLPCoefficients[18], GAMMA_E9);
			weightedqLPCoefficients[19] = MULT16_16_P15(qLPCoefficients[19], GAMMA_E10);

			/*** Compute weighted signal according to spec A3.3.3, this function also compute LPResidualSignal(entire frame values) as specified in eq A.3 ***/
			computeWeightedSpeech(encoderChannelContext->signalCurrentFrame, qLPCoefficients, weightedqLPCoefficients, &(encoderChannelContext->weightedInputSignal[MAXIMUM_INT_PITCH_DELAY]), residualSignal); /* weightedInputSignal contains MAXIMUM_INT_PITCH_DELAY values from previous frame, points to current frame */

			/* update the target Signal : targetSignal = residualSignal - excitationVector */
			for (subframeIndex=0; subframeIndex<L_FRAME; subframeIndex+=L_SUBFRAME) {
				for (i=0; i<L_SUBFRAME; i++) {
					encoderChannelContext->targetSignal[NB_LSP_COEFF+i] = SUB16(residualSignal[subframeIndex+i], encoderChannelContext->excitationVector[L_PAST_EXCITATION+subframeIndex+i]);
				}
				synthesisFilter(&(encoderChannelContext->targetSignal[NB_LSP_COEFF]), &(weightedqLPCoefficients[LPCoefficientsIndex]), &(encoderChannelContext->targetSignal[NB_LSP_COEFF]));
				LPCoefficientsIndex+= NB_LSP_COEFF;
			}

			/***  memory updates                                                        ***/
			/* shift left by L_FRAME the signal buffer */
			memmove(encoderChannelContext->signalBuffer, &(encoderChannelContext->signalBuffer[L_FRAME]), (L_LP_ANALYSIS_WINDOW-L_FRAME)*sizeof(word16_t));
			/* shift left by L_FRAME the weightedInputSignal buffer */
			memmove(encoderChannelContext->weightedInputSignal, &(encoderChannelContext->weightedInputSignal[L_FRAME]), MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t));
			/* shift left by L_FRAME the excitationVector */
			memmove(encoderChannelContext->excitationVector, &(encoderChannelContext->excitationVector[L_FRAME]), L_PAST_EXCITATION*sizeof(word16_t));

			return;
		}
	}

	/* set generated bitStream length: active voice is compressed into 80 bits */
	*bitStreamLength = 10;

	/*********** VAD *****************/





	/*** LSPQuantization and compute L0, L1, L2, L3: the first four parameters ***/
	LSPQuantization(encoderChannelContext, LSPCoefficients, qLSPCoefficients, parameters);
	
	/*** interpolate qLSP and convert to LP ***/
	interpolateqLSP(encoderChannelContext->previousqLSPCoefficients, qLSPCoefficients, interpolatedqLSP);
	/* copy the currentqLSP to previousqLSP buffer */
	for (i=0; i<NB_LSP_COEFF; i++) {
		encoderChannelContext->previousqLSPCoefficients[i] = qLSPCoefficients[i];
	}

	/* first subframe */
	qLSP2LP(interpolatedqLSP, qLPCoefficients);
	/* second subframe */
	qLSP2LP(qLSPCoefficients, &(qLPCoefficients[NB_LSP_COEFF]));

	/*** Compute the weighted Quantized LP Coefficients according to spec A3.3.3 ***/
	/*  weightedqLPCoefficients[0] = qLPCoefficients[0]*Gamma^(i+1) (i=0..9) with Gamma = 0.75 in Q15 */
	weightedqLPCoefficients[0] = MULT16_16_P15(qLPCoefficients[0], GAMMA_E1);
	weightedqLPCoefficients[1] = MULT16_16_P15(qLPCoefficients[1], GAMMA_E2);
	weightedqLPCoefficients[2] = MULT16_16_P15(qLPCoefficients[2], GAMMA_E3);
	weightedqLPCoefficients[3] = MULT16_16_P15(qLPCoefficients[3], GAMMA_E4);
	weightedqLPCoefficients[4] = MULT16_16_P15(qLPCoefficients[4], GAMMA_E5);
	weightedqLPCoefficients[5] = MULT16_16_P15(qLPCoefficients[5], GAMMA_E6);
	weightedqLPCoefficients[6] = MULT16_16_P15(qLPCoefficients[6], GAMMA_E7);
	weightedqLPCoefficients[7] = MULT16_16_P15(qLPCoefficients[7], GAMMA_E8);
	weightedqLPCoefficients[8] = MULT16_16_P15(qLPCoefficients[8], GAMMA_E9);
	weightedqLPCoefficients[9] = MULT16_16_P15(qLPCoefficients[9], GAMMA_E10);
	weightedqLPCoefficients[10] = MULT16_16_P15(qLPCoefficients[10], GAMMA_E1);
	weightedqLPCoefficients[11] = MULT16_16_P15(qLPCoefficients[11], GAMMA_E2);
	weightedqLPCoefficients[12] = MULT16_16_P15(qLPCoefficients[12], GAMMA_E3);
	weightedqLPCoefficients[13] = MULT16_16_P15(qLPCoefficients[13], GAMMA_E4);
	weightedqLPCoefficients[14] = MULT16_16_P15(qLPCoefficients[14], GAMMA_E5);
	weightedqLPCoefficients[15] = MULT16_16_P15(qLPCoefficients[15], GAMMA_E6);
	weightedqLPCoefficients[16] = MULT16_16_P15(qLPCoefficients[16], GAMMA_E7);
	weightedqLPCoefficients[17] = MULT16_16_P15(qLPCoefficients[17], GAMMA_E8);
	weightedqLPCoefficients[18] = MULT16_16_P15(qLPCoefficients[18], GAMMA_E9);
	weightedqLPCoefficients[19] = MULT16_16_P15(qLPCoefficients[19], GAMMA_E10);

	/*** Compute weighted signal according to spec A3.3.3, this function also set LPResidualSignal(entire frame values) as specified in eq A.3 in excitationVector[L_PAST_EXCITATION] ***/
	computeWeightedSpeech(encoderChannelContext->signalCurrentFrame, qLPCoefficients, weightedqLPCoefficients, &(encoderChannelContext->weightedInputSignal[MAXIMUM_INT_PITCH_DELAY]), &(encoderChannelContext->excitationVector[L_PAST_EXCITATION])); /* weightedInputSignal contains MAXIMUM_INT_PITCH_DELAY values from previous frame, points to current frame  */

	/*** find the open loop pitch delay ***/
	openLoopPitchDelay = findOpenLoopPitchDelay(&(encoderChannelContext->weightedInputSignal[MAXIMUM_INT_PITCH_DELAY]));

	/* define boundaries for closed loop pitch delay search as specified in 3.7 */
	intPitchDelayMin = openLoopPitchDelay-3;
	if (intPitchDelayMin < 20) {
		intPitchDelayMin = 20;
	}
	intPitchDelayMax = intPitchDelayMin + 6;
	if (intPitchDelayMax > MAXIMUM_INT_PITCH_DELAY) {
		intPitchDelayMax = MAXIMUM_INT_PITCH_DELAY;
		intPitchDelayMin = MAXIMUM_INT_PITCH_DELAY - 6;
	}

	/*****************************************************************************************/
	/* loop over the two subframes: Closed-loop pitch search(adaptative codebook), fixed codebook, memory update */
	/* set index and buffers */
	impulseResponseInput[0] = ONE_IN_Q12;
	memset(&(impulseResponseInput[1]), 0, (L_SUBFRAME-1)*sizeof(word16_t));

	for (subframeIndex=0; subframeIndex<L_FRAME; subframeIndex+=L_SUBFRAME) {
		int16_t intPitchDelay, fracPitchDelay;
		word16_t adaptativeCodebookGain;
		/*** Compute the impulse response : filter a subframe long buffer filled with unit and only zero through the 1/weightedqLPCoefficients as in spec A.3.5 ***/
		word16_t impulseResponseBuffer[NB_LSP_COEFF+L_SUBFRAME]; /* impulseResponseBuffer in Q12, need NB_LSP_COEFF as past value to go through filtering function */
		word16_t filteredAdaptativeCodebookVector[NB_LSP_COEFF+L_SUBFRAME]; /* in Q0, the first NB_LSP_COEFF words are set to zero and used by filter only */
		word64_t gainQuantizationXy, gainQuantizationYy; /* used to store in Q0 values reused in gain quantization */
		word16_t fixedCodebookVector[L_SUBFRAME]; /* in Q13 */
		word16_t convolvedFixedCodebookVector[L_SUBFRAME]; /* in Q12 */
		word16_t quantizedAdaptativeCodebookGain; /* in Q14 */
		word16_t quantizedFixedCodebookGain; /* in Q1 */

		memset(impulseResponseBuffer, 0, (NB_LSP_COEFF)*sizeof(word16_t)); /* set the past values to zero */
		synthesisFilter(impulseResponseInput, &(weightedqLPCoefficients[LPCoefficientsIndex]), &(impulseResponseBuffer[NB_LSP_COEFF]));
	
		/*** Compute the target signal (x[n]) as in spec A.3.6 in Q0 ***/
		/* excitationVector[L_PAST_EXCITATION+subframeIndex] currently store in Q0 the LPResidualSignal as in spec A.3.3 eq A.3*/
		synthesisFilter( &(encoderChannelContext->excitationVector[L_PAST_EXCITATION+subframeIndex]), &(weightedqLPCoefficients[LPCoefficientsIndex]), &(encoderChannelContext->targetSignal[NB_LSP_COEFF]));

		/*** Adaptative Codebook search : compute the intPitchDelay, fracPitchDelay and associated parameter, compute also the adaptative codebook vector used to generate the excitation ***/
		/* after this call, the excitationVector[L_PAST_EXCITATION + subFrameIndex] contains the adaptative codebook vector as in spec 3.7.1 */
		adaptativeCodebookSearch(&(encoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex]), &intPitchDelayMin, &intPitchDelayMax, &(impulseResponseBuffer[NB_LSP_COEFF]), &(encoderChannelContext->targetSignal[NB_LSP_COEFF]),
			&intPitchDelay, &fracPitchDelay, &(parameters[parametersIndex]), subframeIndex);

		/*** Compute adaptative codebook gain spec 3.7.3, result in Q14 ***/
		/* compute the filtered adaptative codebook vector spec 3.7.3 */
		/* this computation makes use of two partial results used for gainQuantization too (yy and xy in eq63), they are part of the function output */
		/* note spec 3.7.3 eq44 make use of convolution of impulseResponse and adaptative codebook vector to compute the filtered version */
		/* in the Annex A, the filter being simpler, it's faster to directly filter the the vector using the  weightedqLPCoefficients */
		memset(filteredAdaptativeCodebookVector, 0, NB_LSP_COEFF*sizeof(word16_t));
		synthesisFilter(&(encoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex]), &(weightedqLPCoefficients[LPCoefficientsIndex]), &(filteredAdaptativeCodebookVector[NB_LSP_COEFF]));

		adaptativeCodebookGain = computeAdaptativeCodebookGain(&(encoderChannelContext->targetSignal[NB_LSP_COEFF]), &(filteredAdaptativeCodebookVector[NB_LSP_COEFF]), &gainQuantizationXy, &gainQuantizationYy); /* gain in Q14 */
		
		/* increase parameters index and compute P0 if needed */
		parametersIndex++;
		if (subframeIndex==0) { /* first subframe compute P0, the parity bit of P1 */
			parameters[parametersIndex] = computeParity(parameters[parametersIndex-1]);
			parametersIndex++;
		}

		/*** Fixed Codebook Search : compute the parameters for fixed codebook and the regular and convolved version of the fixed codebook vector ***/
		fixedCodebookSearch(&(encoderChannelContext->targetSignal[NB_LSP_COEFF]), &(impulseResponseBuffer[NB_LSP_COEFF]), intPitchDelay, encoderChannelContext->lastQuantizedAdaptativeCodebookGain, &(filteredAdaptativeCodebookVector[NB_LSP_COEFF]), adaptativeCodebookGain,
			&(parameters[parametersIndex]), &(parameters[parametersIndex+1]), fixedCodebookVector, convolvedFixedCodebookVector);
		parametersIndex+=2;

		/*** gains Quantization ***/
		gainQuantization(encoderChannelContext, &(encoderChannelContext->targetSignal[NB_LSP_COEFF]), &(filteredAdaptativeCodebookVector[NB_LSP_COEFF]), convolvedFixedCodebookVector, fixedCodebookVector, gainQuantizationXy, gainQuantizationYy,
			&quantizedAdaptativeCodebookGain, &quantizedFixedCodebookGain, &(parameters[parametersIndex]), &(parameters[parametersIndex+1]));
		parametersIndex+=2;
		
		/*** subframe basis indexes and memory updates ***/
		LPCoefficientsIndex+= NB_LSP_COEFF;
		encoderChannelContext->lastQuantizedAdaptativeCodebookGain = quantizedAdaptativeCodebookGain;
		if (encoderChannelContext->lastQuantizedAdaptativeCodebookGain>ONE_POINT_2_IN_Q14) encoderChannelContext->lastQuantizedAdaptativeCodebookGain = ONE_POINT_2_IN_Q14;
		if (encoderChannelContext->lastQuantizedAdaptativeCodebookGain<O2_IN_Q14) encoderChannelContext->lastQuantizedAdaptativeCodebookGain = O2_IN_Q14;
		/* compute excitation for current subframe as in spec A.3.10 */
		/* excitationVector[L_PAST_EXCITATION + subframeIndex] currently contains in Q0 the adaptative codebook vector, quantizedAdaptativeCodebookGain in Q14 */
		/* fixedCodebookVector in Q13, quantizedFixedCodebookGain in Q1 */
		for (i=0; i<L_SUBFRAME; i++) {
			encoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex + i] = (word16_t)(SATURATE(PSHR(ADD32(MULT16_16(encoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex + i], quantizedAdaptativeCodebookGain),
											MULT16_16(fixedCodebookVector[i], quantizedFixedCodebookGain)), 14), MAXINT16)); /* result in Q0 */
		}

		/* update targetSignal memory as in spec A.3.10 */
		quantizedAdaptativeCodebookGain = PSHR(quantizedAdaptativeCodebookGain, 1); /* quantizedAdaptativeCodebookGain in Q13 */
		for (i=0; i<NB_LSP_COEFF; i++) {
			/* targetSignal[i] = targetSignal[L_SUBFRAME+i] - quantizedAdaptativeCodebookGain*filteredAdaptativeCodebookVector[L_SUBFRAME+i] - quantizedFixedCodebookGain*convolvedFixedCodebookVector[L_SUBFRAME-NB_LSP_COEFF+i]*/
			word32_t acc = MAC16_16(MULT16_16(quantizedAdaptativeCodebookGain, filteredAdaptativeCodebookVector[L_SUBFRAME+i]), quantizedFixedCodebookGain, convolvedFixedCodebookVector[L_SUBFRAME-NB_LSP_COEFF+i]); /* acc in Q13 */
			encoderChannelContext->targetSignal[i] = (word16_t)(SATURATE(SUB32(encoderChannelContext->targetSignal[L_SUBFRAME+i], PSHR(acc, 13)), MAXINT16));
			
		}
	}

	/*****************************************************************************************/
	/*** frame basis memory updates                                                        ***/
	/* shift left by L_FRAME the signal buffer */
	memmove(encoderChannelContext->signalBuffer, &(encoderChannelContext->signalBuffer[L_FRAME]), (L_LP_ANALYSIS_WINDOW-L_FRAME)*sizeof(word16_t)); 
	/* update previousLSP coefficient buffer */
	memcpy(encoderChannelContext->previousLSPCoefficients, LSPCoefficients, NB_LSP_COEFF*sizeof(word16_t));
	memcpy(encoderChannelContext->previousqLSPCoefficients, qLSPCoefficients, NB_LSP_COEFF*sizeof(word16_t));
	/* shift left by L_FRAME the weightedInputSignal buffer */
	memmove(encoderChannelContext->weightedInputSignal, &(encoderChannelContext->weightedInputSignal[L_FRAME]), MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t));
	/* shift left by L_FRAME the excitationVector */
	memmove(encoderChannelContext->excitationVector, &(encoderChannelContext->excitationVector[L_FRAME]), L_PAST_EXCITATION*sizeof(word16_t));

	/*** Convert array of parameters into bitStream ***/
	parametersArray2BitStream(parameters, bitStream);

	return;
}

/*****************************************************************************/
/* bcg729GetRFC3389Payload : return the comfort noise payload according to   */
/*                     RFC3389 for the last CN frame generated by encoder    */
/*                                                                           */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : retrieve the last CN frame encoded      */
/*                    using this context                                     */
/*      -(o) payload : 11 parameters following RFC3389 with filter order 10  */
/*                                                                           */
/*****************************************************************************/
void bcg729GetRFC3389Payload(bcg729EncoderChannelContextStruct *encoderChannelContext, uint8_t payload[]) {
	int i;

	/* decodedLogEnergy is the frame mean energy, range [-12,66[, RFC3389 ask for DBov value, max mean energy would be 10log10(2^15*2^15) -> 90 so -90 to switch to DBov */
	payload[0] = -(encoderChannelContext->DTXChannelContext->decodedLogEnergy - 90);

	/* use last reflection coefficients stored in DTX context and convert them as specified in RFC3389 : Ni = (ki in Q15)/258 + 127 */
	for (i=0; i<NB_LSP_COEFF; i++) {
		payload[i+1] = (uint8_t)ADD16((word16_t)SHR(MULT16_32_Q15(127/* 1/258 in Q15 */, -encoderChannelContext->DTXChannelContext->reflectionCoefficients[i]), 16), 127); /* ki in Q31 * (1/258 in Q15) -> result in Q31, shift right by 16 -> Q15 */
	}
}
