/*
 decoder.c

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

#include "bcg729/decoder.h"
#include "decodeLSP.h"
#include "interpolateqLSP.h"
#include "qLSP2LP.h"
#include "decodeAdaptativeCodeVector.h"
#include "decodeFixedCodeVector.h"
#include "decodeGains.h"
#include "LPSynthesisFilter.h"
#include "postFilter.h"
#include "postProcessing.h"
#include "cng.h"

#include "stdio.h"

/* buffers allocation */
static const word16_t previousqLSPInitialValues[NB_LSP_COEFF] = {30000, 26000, 21000, 15000, 8000, 0, -8000,-15000,-21000,-26000}; /* in Q0.15 the initials values for the previous qLSP buffer */

/*****************************************************************************/
/* initBcg729DecoderChannel : create context structure and initialise it     */
/*    return value :                                                         */
/*      - the decoder channel context data                                   */
/*                                                                           */
/*****************************************************************************/
bcg729DecoderChannelContextStruct *initBcg729DecoderChannel()
{
	/* create the context structure */
	bcg729DecoderChannelContextStruct *decoderChannelContext = malloc(sizeof(bcg729DecoderChannelContextStruct));
	memset(decoderChannelContext, 0, sizeof(bcg729DecoderChannelContextStruct));

	/* intialise statics buffers and variables */
	memcpy(decoderChannelContext->previousqLSP, previousqLSPInitialValues, NB_LSP_COEFF*sizeof(word16_t)); /* initialise the previousqLSP buffer */
	memset(decoderChannelContext->excitationVector, 0, L_PAST_EXCITATION*sizeof(word16_t)); /* initialise the part of the excitationVector containing the past excitation */
	decoderChannelContext->boundedAdaptativeCodebookGain = BOUNDED_PITCH_GAIN_MIN;
	decoderChannelContext->pseudoRandomSeed = 21845; /* initialise pseudo Random seed according to spec 4.4.4 */
	decoderChannelContext->CNGpseudoRandomSeed = CNG_DTX_RANDOM_SEED_INIT; /* initialise CNG pseudo Random seed according to ITU code */
	decoderChannelContext->adaptativeCodebookGain = 0; /* gains are initialised at 0 */
	decoderChannelContext->fixedCodebookGain = 0;
	memset(decoderChannelContext->reconstructedSpeech, 0, NB_LSP_COEFF*sizeof(word16_t)); /* initialise to zero all the values used from previous frame to get the current frame reconstructed speech */
	decoderChannelContext->previousFrameIsActiveFlag = 1;
	decoderChannelContext->CNGChannelContext = initBcg729CNGChannel();


	/* initialisation of the differents blocs which need to be initialised */
	initDecodeLSP(decoderChannelContext);
	initDecodeAdaptativeCodeVector(decoderChannelContext);
	initDecodeGains(decoderChannelContext);
	initPostFilter(decoderChannelContext);
	initPostProcessing(decoderChannelContext);

	return decoderChannelContext;
}

/*****************************************************************************/
/* closeBcg729DecoderChannel : free memory of context structure              */
/*    parameters:                                                            */
/*      -(i) decoderChannelContext : the channel context data                */
/*                                                                           */
/*****************************************************************************/
void closeBcg729DecoderChannel(bcg729DecoderChannelContextStruct *decoderChannelContext)
{
	if (decoderChannelContext) {
		if (decoderChannelContext->CNGChannelContext) {
			free(decoderChannelContext->CNGChannelContext);
		}
		free(decoderChannelContext);
	}
	return;
}

/*****************************************************************************/
/* bcg729Decoder :                                                           */
/*    parameters:                                                            */
/*      -(i) decoderChannelContext : the channel context data                */
/*      -(i) bitStream : 15 parameters on 80 bits                            */
/*      -(i): bitStreamLength : in bytes, length of previous buffer          */
/*      -(i) frameErased: flag: true, frame has been erased                  */
/*      -(i) SIDFrameFlag: flag: true, frame is a SID one                    */
/*      -(i) rfc3389PayloadFlag: true when CN payload follow rfc3389         */
/*      -(o) signal : a decoded frame 80 samples (16 bits PCM)               */
/*                                                                           */
/*****************************************************************************/
void bcg729Decoder(bcg729DecoderChannelContextStruct *decoderChannelContext, uint8_t bitStream[], uint8_t bitStreamLength, uint8_t frameErasureFlag, uint8_t SIDFrameFlag, uint8_t rfc3389PayloadFlag, int16_t signal[])
{
	int i;
	uint16_t parameters[NB_PARAMETERS];
	/* internal buffers which we do not need to keep between calls */
	word16_t qLSP[NB_LSP_COEFF]; /* store the qLSP coefficients in Q0.15 */
	word16_t interpolatedqLSP[NB_LSP_COEFF]; /* store the interpolated qLSP coefficient in Q0.15 */
	word16_t LP[2*NB_LSP_COEFF]; /* store the 2 sets of LP coefficients in Q12 */
	int16_t intPitchDelay; /* store the Pitch Delay in and out of decodeAdaptativeCodeVector, in for decodeFixedCodeVector */
	word16_t fixedCodebookVector[L_SUBFRAME]; /* the fixed Codebook Vector in Q1.13*/
	word16_t postFilteredSignal[L_SUBFRAME]; /* store the postfiltered signal in Q0 */

	uint8_t parityErrorFlag;
	int subframeIndex;
	int parametersIndex = 4; /* this is used to select the right parameter according to the subframe currently computed, start pointing to P1 */
	int LPCoefficientsIndex = 0; /* this is used to select the right LP Coefficients according to the subframe currently computed */

	/*** parse the bitstream and get all parameter into an array as in spec 4 - Table 8 ***/
	/* parameters buffer mapping : */
	/* 0 -> L0 (1 bit)             */
	/* 1 -> L1 (7 bits)            */
	/* 2 -> L2 (5 bits)            */
	/* 3 -> L3 (5 bits)            */
	/* 4 -> P1 (8 bit)             */
	/* 5 -> P0 (1 bits)            */ 
	/* 6 -> C1 (13 bits)           */
	/* 7 -> S1 (4 bits)            */
	/* 8 -> GA1(3 bits)            */
	/* 9 -> GB1(4 bits)            */
	/* 10 -> P2 (5 bits)           */
	/* 11 -> C2 (13 bits)          */
	/* 12 -> S2 (4 bits)           */
	/* 13 -> GA2(3 bits)           */
	/* 14 -> GB2(4 bits)           */
	/* in case of SID frame : params are decoded in the decodeSIDframe functions */
	/* 0-> L0 (1 bit)              */
	/* 1-> L1 (5 bits)             */
	/* 2-> L2 (4 bits)             */
	/* 3-> Gain (5 bits)           */
	if (bitStream!=NULL) { /* bitStream might be null in case of frameErased (which shall be set in the appropriated flag)*/
		if (SIDFrameFlag == 0) {
			parametersBitStream2Array(bitStream, parameters);
		}
	} else { /* avoid compiler complaining for non inizialazed use of variable */
		for (i=0; i<NB_PARAMETERS; i++) {
			parameters[i]=0;
		}
	}

	/* manage frameErasure and CNG as specified in B.27 */
	if (frameErasureFlag) {
		if (decoderChannelContext->previousFrameIsActiveFlag) {
			SIDFrameFlag = 0;
		} else {
			SIDFrameFlag = 1;
		}
	}

	/* this is a SID frame, process it using the dedicated function */
	if (SIDFrameFlag == 1) {
		decodeSIDframe(decoderChannelContext->CNGChannelContext, decoderChannelContext->previousFrameIsActiveFlag, bitStream, bitStreamLength, &(decoderChannelContext->excitationVector[L_PAST_EXCITATION]), decoderChannelContext->previousqLSP, LP, &(decoderChannelContext->CNGpseudoRandomSeed), decoderChannelContext->previousLCodeWord, rfc3389PayloadFlag);
		decoderChannelContext->previousFrameIsActiveFlag = 0;

		/* loop over the two subframes */
		for (subframeIndex=0; subframeIndex<L_FRAME; subframeIndex+=L_SUBFRAME) {
			/* reconstruct speech using LP synthesis filter spec 4.1.6 eq77 */
			/* excitationVector in Q0, LP in Q12, recontructedSpeech in Q0 -> +NB_LSP_COEFF on the index of this one because the first NB_LSP_COEFF elements store the previous frame filter output */
			LPSynthesisFilter(&(decoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex]), &(LP[LPCoefficientsIndex]), &(decoderChannelContext->reconstructedSpeech[NB_LSP_COEFF+subframeIndex]) );

			/* NOTE: ITU code check for overflow after LP Synthesis Filter computation and if it happened, divide excitation buffer by 2 and recompute the LP Synthesis Filter */
			/*	here, possible overflows are managed directly inside the Filter by saturation at MAXINT16 on each result */

			/* postFilter */
			postFilter(decoderChannelContext, &(LP[LPCoefficientsIndex]), /* select the LP coefficients for this subframe, use last frame intPitchDelay */
				&(decoderChannelContext->reconstructedSpeech[NB_LSP_COEFF+subframeIndex]), decoderChannelContext->previousIntPitchDelay, subframeIndex, postFilteredSignal);

			/* postProcessing */
			postProcessing(decoderChannelContext, postFilteredSignal);

			/* copy postProcessing Output to the signal output buffer */
			for (i=0; i<L_SUBFRAME; i++) {
				signal[subframeIndex+i] = postFilteredSignal[i];
			}

			/* increase LPCoefficient Indexes */
			LPCoefficientsIndex+=NB_LSP_COEFF;
		}

		decoderChannelContext->boundedAdaptativeCodebookGain = BOUNDED_PITCH_GAIN_MIN;

		/* Shift Excitation Vector by L_FRAME left */
		memmove(decoderChannelContext->excitationVector, &(decoderChannelContext->excitationVector[L_FRAME]), L_PAST_EXCITATION*sizeof(word16_t));
		/* Copy the last 10 words of reconstructed Speech to the begining of the array for next frame computation */
		memcpy(decoderChannelContext->reconstructedSpeech, &(decoderChannelContext->reconstructedSpeech[L_FRAME]), NB_LSP_COEFF*sizeof(word16_t));

		return;
	}

	decoderChannelContext->previousFrameIsActiveFlag = 1;
	/* re-init the CNG pseudo random seed at each active frame spec B.4 */
	decoderChannelContext->CNGpseudoRandomSeed = CNG_DTX_RANDOM_SEED_INIT; /* re-initialise CNG pseudo Random seed to 11111 according to ITU code */


	/*****************************************************************************************/
	/*** on frame basis : decodeLSP, interpolate them with previous ones and convert to LP ***/
	decodeLSP(decoderChannelContext, parameters, qLSP, frameErasureFlag); /* decodeLSP need the first 4 parameters: L0-L3 */


	interpolateqLSP(decoderChannelContext->previousqLSP, qLSP, interpolatedqLSP);
	/* copy the currentqLSP to previousqLSP buffer */
	for (i=0; i<NB_LSP_COEFF; i++) {
		decoderChannelContext->previousqLSP[i] = qLSP[i];
	}

	/* call the qLSP2LP function for first subframe */
	qLSP2LP(interpolatedqLSP, LP);
	/* call the qLSP2LP function for second subframe */
	qLSP2LP(qLSP, &(LP[NB_LSP_COEFF]));

	/* check the parity on the adaptativeCodebookIndexSubframe1(P1) with the received one (P0)*/
	parityErrorFlag = (uint8_t)(computeParity(parameters[4]) ^ parameters[5]);

	/* loop over the two subframes */
	for (subframeIndex=0; subframeIndex<L_FRAME; subframeIndex+=L_SUBFRAME) {
		/* decode the adaptative Code Vector */	
		decodeAdaptativeCodeVector(	decoderChannelContext,
						subframeIndex,
						parameters[parametersIndex], 
						parityErrorFlag,
						frameErasureFlag,
						&intPitchDelay,

						&(decoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex]));
		if (subframeIndex==0) { /* at first subframe we have P0 between P1 and C1 */
			parametersIndex+=2;
		} else {
			parametersIndex++;
		}

		/* in case of frame erasure we shall generate pseudoRandom signs and index for fixed code vector decoding according to spec 4.4.4 */
		if (frameErasureFlag) {
			parameters[parametersIndex] = pseudoRandom(&(decoderChannelContext->pseudoRandomSeed))&(uint16_t)0x1fff; /* signs are set to the 13 LSB of the first pseudoRandom number */
			parameters[parametersIndex+1] = pseudoRandom(&(decoderChannelContext->pseudoRandomSeed))&(uint16_t)0x000f; /* signs are set to the 4 LSB of the second pseudoRandom number */
		}

		/* decode the fixed Code Vector */
		decodeFixedCodeVector(parameters[parametersIndex+1], parameters[parametersIndex], intPitchDelay, decoderChannelContext->boundedAdaptativeCodebookGain, fixedCodebookVector);
		parametersIndex+=2;

		/* decode gains */
		decodeGains(decoderChannelContext, parameters[parametersIndex], parameters[parametersIndex+1], fixedCodebookVector, frameErasureFlag,
				&(decoderChannelContext->adaptativeCodebookGain), &(decoderChannelContext->fixedCodebookGain));
		parametersIndex+=2;

		/* update bounded Adaptative Codebook Gain (in Q14) according to eq47 */
		decoderChannelContext->boundedAdaptativeCodebookGain = decoderChannelContext->adaptativeCodebookGain;
		if (decoderChannelContext->boundedAdaptativeCodebookGain>BOUNDED_PITCH_GAIN_MAX) {
			decoderChannelContext->boundedAdaptativeCodebookGain = BOUNDED_PITCH_GAIN_MAX;
		}
		if (decoderChannelContext->boundedAdaptativeCodebookGain<BOUNDED_PITCH_GAIN_MIN) {
			decoderChannelContext->boundedAdaptativeCodebookGain = BOUNDED_PITCH_GAIN_MIN;
		}

		/* compute excitation vector according to eq75 */
		/* excitationVector = adaptative Codebook Vector * adaptativeCodebookGain + fixed Codebook Vector * fixedCodebookGain */
		/* the adaptative Codebook Vector is in the excitationVector buffer [L_PAST_EXCITATION + subframeIndex] */
		/* with adaptative Codebook Vector in Q0, adaptativeCodebookGain in Q14, fixed Codebook Vector in Q1.13 and fixedCodebookGain in Q14.1 -> result in Q14 on 32 bits */
		/* -> shift right 14 bits and store the value in Q0 in a 16 bits type */
		for (i=0; i<L_SUBFRAME; i++) {
			decoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex + i] = (word16_t)(SATURATE(PSHR(
				ADD32(
					MULT16_16(decoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex + i], decoderChannelContext->adaptativeCodebookGain),
					MULT16_16(fixedCodebookVector[i], decoderChannelContext->fixedCodebookGain)
				     ), 14), MAXINT16));
		}

		/* reconstruct speech using LP synthesis filter spec 4.1.6 eq77 */
		/* excitationVector in Q0, LP in Q12, recontructedSpeech in Q0 -> +NB_LSP_COEFF on the index of this one because the first NB_LSP_COEFF elements store the previous frame filter output */
		LPSynthesisFilter(&(decoderChannelContext->excitationVector[L_PAST_EXCITATION + subframeIndex]), &(LP[LPCoefficientsIndex]), &(decoderChannelContext->reconstructedSpeech[NB_LSP_COEFF+subframeIndex]) );

		/* NOTE: ITU code check for overflow after LP Synthesis Filter computation and if it happened, divide excitation buffer by 2 and recompute the LP Synthesis Filter */
		/*	here, possible overflows are managed directly inside the Filter by saturation at MAXINT16 on each result */ 

		/* postFilter */
		postFilter(decoderChannelContext, &(LP[LPCoefficientsIndex]), /* select the LP coefficients for this subframe */
				&(decoderChannelContext->reconstructedSpeech[NB_LSP_COEFF+subframeIndex]), intPitchDelay, subframeIndex, postFilteredSignal);

		/* postProcessing */
		postProcessing(decoderChannelContext, postFilteredSignal);

		/* copy postProcessing Output to the signal output buffer */
		for (i=0; i<L_SUBFRAME; i++) {
			signal[subframeIndex+i] = postFilteredSignal[i]; 
		}

		/* increase LPCoefficient Indexes */
		LPCoefficientsIndex+=NB_LSP_COEFF;
	}

	/* Shift Excitation Vector by L_FRAME left */
	memmove(decoderChannelContext->excitationVector, &(decoderChannelContext->excitationVector[L_FRAME]), L_PAST_EXCITATION*sizeof(word16_t)); 
	/* Copy the last 10 words of reconstructed Speech to the begining of the array for next frame computation */
	memcpy(decoderChannelContext->reconstructedSpeech, &(decoderChannelContext->reconstructedSpeech[L_FRAME]), NB_LSP_COEFF*sizeof(word16_t));

	return;
}
