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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

/* buffers allocation */
static const word16_t previousqLSPInitialValues[NB_LSP_COEFF] = {30000, 26000, 21000, 15000, 8000, 0, -8000,-15000,-21000,-26000}; /* in Q0.15 the initials values for the previous qLSP buffer */

/* internal functions */
uint16_t pseudoRandom(bcg729DecoderChannelContextStruct *decoderChannelContext);

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

	/* intialise statics buffers and variables */
	memcpy(decoderChannelContext->previousqLSP, previousqLSPInitialValues, NB_LSP_COEFF*sizeof(word16_t)); /* initialise the previousqLSP buffer */
	memset(decoderChannelContext->excitationVector, 0, L_PAST_EXCITATION*sizeof(word16_t)); /* initialise the part of the excitationVector containing the past excitation */
	decoderChannelContext->boundedAdaptativeCodebookGain = BOUNDED_PITCH_GAIN_MIN;
	decoderChannelContext->pseudoRandomSeed = 21845; /* initialise pseudo Random seed according to spec 4.4.4 */
	decoderChannelContext->adaptativeCodebookGain = 0; /* gains are initialised at 0 */
	decoderChannelContext->fixedCodebookGain = 0;
	memset(decoderChannelContext->reconstructedSpeech, 0, NB_LSP_COEFF*sizeof(word16_t)); /* initialise to zero all the values used from previous frame to get the current frame reconstructed speech */

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
	free(decoderChannelContext);
	return;
}

/*****************************************************************************/
/* bcg729Decoder :                                                           */
/*    parameters:                                                            */
/*      -(i) decoderChannelContext : the channel context data                */
/*      -(i) bitStream : 15 parameters on 80 bits                            */
/*      -(i) frameErased: flag: true, frame has been erased                  */
/*      -(o) signal : a decoded frame 80 samples (16 bits PCM)               */
/*                                                                           */
/*****************************************************************************/
void bcg729Decoder(bcg729DecoderChannelContextStruct *decoderChannelContext, uint8_t bitStream[], uint8_t frameErasureFlag, int16_t signal[])
{
	int i;
	uint16_t parameters[NB_PARAMETERS];

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
	if (bitStream!=NULL) { /* bitStream might be null in case of frameErased (which shall be set in the appropriated flag)*/
		parametersBitStream2Array(bitStream, parameters);
	} else { /* avoid compiler complaining for non inizialazed use of variable */
		for (i=0; i<NB_PARAMETERS; i++) {
			parameters[i]=0;
		}
	}

	/* internal buffers which we do not need to keep between calls */
	word16_t qLSP[NB_LSP_COEFF]; /* store the qLSP coefficients in Q0.15 */
	word16_t interpolatedqLSP[NB_LSP_COEFF]; /* store the interpolated qLSP coefficient in Q0.15 */
	word16_t LP[2*NB_LSP_COEFF]; /* store the 2 sets of LP coefficients in Q12 */
	int16_t intPitchDelay; /* store the Pitch Delay in and out of decodeAdaptativeCodeVector, in for decodeFixedCodeVector */
	word16_t fixedCodebookVector[L_SUBFRAME]; /* the fixed Codebook Vector in Q1.13*/
	word16_t postFilteredSignal[L_SUBFRAME]; /* store the postfiltered signal in Q0 */


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
	uint8_t parityErrorFlag = (uint8_t)(computeParity(parameters[4]) ^ parameters[5]);

	/* loop over the two subframes */
	int subframeIndex;
	int parametersIndex = 4; /* this is used to select the right parameter according to the subframe currently computed, start pointing to P1 */
	int LPCoefficientsIndex = 0; /* this is used to select the right LP Coefficients according to the subframe currently computed */
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
			parameters[parametersIndex] = pseudoRandom(decoderChannelContext)&(uint16_t)0x1fff; /* signs are set to the 13 LSB of the first pseudoRandom number */
			parameters[parametersIndex+1] = pseudoRandom(decoderChannelContext)&(uint16_t)0x000f; /* signs are set to the 4 LSB of the second pseudoRandom number */
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


/*****************************************************************************/
/* pseudoRandom : generate pseudo random number as in spec 4.4.4 eq96        */
/*    parameters:                                                            */
/*      -(i) decoderChannelContext : the channel context data                */
/*    return value :                                                         */
/*      - a unsigned 16 bits pseudo random number                            */
/*                                                                           */
/*****************************************************************************/
uint16_t pseudoRandom(bcg729DecoderChannelContextStruct *decoderChannelContext)
{
	/* pseudoRandomSeed is stored in an uint16_t var, we shall not worry about overflow here */
	/* pseudoRandomSeed*31821 + 13849; */
	return decoderChannelContext->pseudoRandomSeed = MAC16_16(13849, (decoderChannelContext->pseudoRandomSeed), 31821);
}
