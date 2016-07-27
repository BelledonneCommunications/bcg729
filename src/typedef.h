/*
 typedef.h

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
#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "codecParameters.h"
#include "bcg729/encoder.h"
#include "bcg729/decoder.h"

typedef int16_t word16_t;
typedef uint16_t uword16_t;
typedef int32_t word32_t;
typedef uint32_t uword32_t;
typedef int64_t word64_t;

struct bcg729VADChannelContextStruct_struct {
	/* buffer used during the init period - first N0(32) frames */
	word32_t initEfSum;
	word32_t initZCSum;
	word32_t initLSFSum[NB_LSP_COEFF];
	uint8_t nbValidInitFrame; /* VAD init needs 32 frames to be completed but we must use only frame with Ef > 15dB so we must count them */


	word16_t meanZC; /* in Q15 */
	word16_t meanEf; /* in Q11 */
	word16_t meanEl; /* in Q11 */
	word16_t meanLSF[NB_LSP_COEFF]; /* in Q2.13 */
	uint32_t frameCount; /* number of frame processed, used to close the initialisation period */
	uint32_t updateCount; /* number of frame leading to an update of running averages */
	word16_t EfBuffer[N0]; /* store the last N0 Ef values to be able to retrieve the min */
	uint8_t SVDm1; /* smoothed voice activity decision on last frame */
	uint8_t SVDm2; /* smoothed voice activity decision on 2 frames ago */
	uint32_t Count_inert; /* defined in appendix II, used to implement hysteresis on VOICE->NOISE switching */
	uint8_t secondStageVADSmoothingFlag; /* flag defined as FVD-1 in spec B3.6 */
	uint32_t smoothingCounter; /* counter Ce defined in spec B3.6 */
	word16_t previousFrameEf; /* in Q11 */
	word32_t noiseContinuityCounter; /* counter Cs defined in spec B3.6 */
};
typedef struct bcg729VADChannelContextStruct_struct bcg729VADChannelContextStruct;

struct bcg729DTXChannelContextStruct_struct {
	/* store past autocorrelations coefficients for 6 past frames and current one(at index 0) spec B4.1.1 */
	word32_t autocorrelationCoefficients[7][NB_LSP_COEFF+1];
	int8_t autocorrelationCoefficientsScale[7];

	uint8_t previousVADflag; /* previous frame VAD : 1 VOICE 0 NOISE */
	word32_t previousResidualEnergy;
	uint8_t previousResidualEnergyScale;
	int8_t previousDecodedLogEnergy; /* store the last decodedLog energy sent in a SID frame */
	uint8_t count_fr; /* count the number of noise frame since last SID frame was sent */
	word32_t SIDLPCoefficientAutocorrelation[NB_LSP_COEFF+1]; /* the autocorrelation of LP coefficients as described in eq B.13 in Q20 */
	word16_t currentSIDGain; /* gain in Q3 */
	word16_t smoothedSIDGain; /* gain in Q3 */
	uint16_t pseudoRandomSeed; /* seed used in the pseudo random number generator for excitation generation */
	word16_t qLSPCoefficients[NB_LSP_COEFF]; /* current Quantized LSP coefficient in Q15, saved to be re-used in case of untransmitted frame */
	word32_t reflectionCoefficients[NB_LSP_COEFF]; /* used to generate the RFC3389 payload, generated during LP computation */
	int8_t decodedLogEnergy; /* log10(frame residual energy), used to generate noise level for RFC3389 paylaod */
};
typedef struct bcg729DTXChannelContextStruct_struct bcg729DTXChannelContextStruct;

struct bcg729CNGChannelContextStruct_struct {
	word16_t receivedSIDGain; /* gain in Q3 */
	word16_t smoothedSIDGain; /* gain in Q3 */
	word16_t qLSP[NB_LSP_COEFF]; /* qLSP in Q0.15 */
	word64_t lastFrameEnergy; /* in Q0 */
};

typedef struct bcg729CNGChannelContextStruct_struct bcg729CNGChannelContextStruct;

/* define the context structure to store all static data for a decoder channel */
struct bcg729DecoderChannelContextStruct_struct {
	/*** buffers used in decoder bloc ***/
	word16_t previousqLSP[NB_LSP_COEFF]; /* previous quantised LSP in Q0.15 */
	word16_t excitationVector[L_PAST_EXCITATION + L_FRAME]; /* in Q0 this vector contains: 
		0->153 : the past excitation vector.(length is Max Pitch Delay: 144 + interpolation window size : 10)
		154-> 154+L_FRAME-1 : the current frame adaptative Code Vector first used to compute then the excitation vector */
	word16_t boundedAdaptativeCodebookGain; /* the pitch gain from last subframe bounded in range [0.2,0.8] in Q0.14 */
	word16_t adaptativeCodebookGain; /* the gains needs to be stored in case of frame erasure in Q14 */
	word16_t fixedCodebookGain; /* in Q14.1 */
	word16_t reconstructedSpeech[NB_LSP_COEFF+L_FRAME]; /* in Q0, output of the LP synthesis filter, the first 10 words store the previous frame output */
	uint16_t pseudoRandomSeed; /* seed used in the pseudo random number generator */
	uint16_t CNGpseudoRandomSeed; /* seed used in the pseudo random number generator for CNG */

	/*** buffers used in decodeLSP bloc ***/
	word16_t lastqLSF[NB_LSP_COEFF]; /* this buffer stores the last qLSF to be used in case of frame lost in Q2.13 */
	/* buffer to store the last 4 frames codewords, used to compute the current qLSF */
	word16_t previousLCodeWord[MA_MAX_K][NB_LSP_COEFF]; /* in Q2.13, buffer to store the last 4 frames codewords, used to compute the current qLSF */
		/* the values stored are the codewords computed from the codebooks and rearranged */
	word16_t lastValidL0; /* this one store the L0 of last valid frame to be used in case of frame erased */

	/*** buffer used in decodeAdaptativeCodeVector bloc ***/
	uint16_t previousIntPitchDelay;  /* store the last valid Integer Pitch Delay computed, used in case of parity error or frame erased */

	/*** buffer used in decodeGains bloc ***/
	word16_t previousGainPredictionError[4]; /* the last four gain prediction error U(m) eq69 and eq72, spec3.9.1 in Q10*/

	/*** buffers used in postFilter bloc ***/
	word16_t residualSignalBuffer[MAXIMUM_INT_PITCH_DELAY+L_FRAME]; /* store the residual signal (current subframe and MAXIMUM_INT_PITCH_DELAY of previous values) in Q0 */
	word16_t scaledResidualSignalBuffer[MAXIMUM_INT_PITCH_DELAY+L_FRAME]; /* same as previous but in Q-2 */
	word16_t longTermFilteredResidualSignalBuffer[1+L_SUBFRAME]; /* the output of long term filter in Q0, need 1 word from previous subframe for tilt compensation filter */
	word16_t *longTermFilteredResidualSignal; /* points to the beginning of current subframe longTermFilteredResidualSignal */
	word16_t shortTermFilteredResidualSignalBuffer[NB_LSP_COEFF+L_SUBFRAME]; /* the output of short term filter(synthesis filter) in Q0, need NB_LSP_COEFF word from previous subframe as filter memory */
	word16_t *shortTermFilteredResidualSignal; /* points to the beginning of current subframe shortTermFilteredResidualSignal */
	word16_t previousAdaptativeGain; /* previous gain for adaptative gain control */

	/*** buffers used in postProcessing bloc ***/
	word16_t inputX0;
	word16_t inputX1;
	word32_t outputY2;
	word32_t outputY1;

	/* SID frame management */
	bcg729CNGChannelContextStruct *CNGChannelContext; /* store informations specific to CNG */
	uint8_t previousFrameIsActiveFlag; /* store last processed frame type */

};

/* define the context structure to store all static data for an encoder channel */
struct bcg729EncoderChannelContextStruct_struct {
	/*** buffers used in decoder bloc ***/
	/* Signal buffer mapping : 240 word16_t length */
	/* <----  120 word16_t -->|<----               80 word16_t         ---->|<----       40 word16_t      --->| */
	/* |----- old signal -----|----------- current frame -------------------|-----next subframe 1 ------------| */
	/*                        |----- subframe 1 -----|----- subframe 2 -----|                                   */
	/*                                               |--------------- last input frame -----------------------| */
	/* ^                      ^                      ^                                                          */
	/* |                      |                      |                                                          */
	/* signalBuffer           signalCurrentFrame     signalLastInputFrame                                       */
	word16_t signalBuffer[L_LP_ANALYSIS_WINDOW]; /* this buffer stores the input signal */
	word16_t *signalLastInputFrame; /* point to the beginning of the last frame in the signal buffer */
	word16_t *signalCurrentFrame; /* point to the beginning of the current frame in the signal buffer */
	word16_t previousLSPCoefficients[NB_LSP_COEFF]; /* LSP coefficient of previous frame */
	word16_t previousqLSPCoefficients[NB_LSP_COEFF]; /* Quantized LSP coefficient of previous frame */
	word16_t weightedInputSignal[MAXIMUM_INT_PITCH_DELAY+L_FRAME]; /* buffer storing the weightedInputSignal on current frame and MAXIMUM_INT_PITCH_DELAY of previous values */
	word16_t excitationVector[L_PAST_EXCITATION + L_FRAME]; /* in Q0 this vector contains: 
			0->153 : the past excitation vector.(length is Max Pitch Delay: 144 + interpolation window size : 10)
			154-> 154+L_FRAME-1 : the current frame adaptative Code Vector first used to compute then the excitation vector */
	word16_t targetSignal[NB_LSP_COEFF+L_SUBFRAME]; /* in Q0, buffer holding the target signal (x[n]) as in spec A.3.6, the first NB_LSP_COEFF values are memory from previous subframe used in filtering(computed according to spec A.3.10), the following values are the target signal for current subframe */
	word16_t lastQuantizedAdaptativeCodebookGain; /* in Q14, the quantized adaptive codebook gain from previous subframe */

	/*** buffer used in preProcessing ***/
	word16_t inputX0, inputX1;
	word32_t outputY2, outputY1;

	/*** buffer used in LSPQuantization ***/
	word16_t previousqLSF[MA_MAX_K][NB_LSP_COEFF]; /* previousqLSF of the last 4(MA pred buffer size) frames in Q13, contains actually quantizer output (l) and not LSF (w)*/ 

	/*** buffer used in gainQuantization ***/
	word16_t previousGainPredictionError[4]; /* the last four gain prediction error U(m) eq69 and eq72, spec3.9.1 in Q10*/

	/*** VAD management ***/
	bcg729VADChannelContextStruct *VADChannelContext;
	bcg729DTXChannelContextStruct *DTXChannelContext;
};

/* MAXINTXX define the maximum signed integer value on XX bits(2^(XX-1) - 1) */
/* used to check on overflows in fixed point mode */
#define MAXINT16 0x7fff
#define MAXINT28 0x7ffffff
#define MAXINT29 0xfffffff
#define MININT32 0x80000000
#define MAXINT32 0x7fffffff
#define MAXINT64 0x7fffffffffffffffLL

/* several values used for inits */
#define ONE_IN_Q31 0x7FFFFFFF
#define ONE_IN_Q30 0x40000000
#define ONE_IN_Q27 0x08000000
#define ONE_IN_Q15 0x00008000
#define ONE_IN_Q13 0x00002000
#define ONE_IN_Q12 0x00001000
#define ONE_IN_Q11 0x00000800

#define HALF_PI_Q13 12868 
#define HALF_PI_Q15 51472

/* 0.04*Pi + 1 and 0.92*Pi - 1 used by LSPQuantization */
#define OO4PIPLUS1_IN_Q13 9221
#define O92PIMINUS1_IN_Q13 15485
/* 1.2 in Q14 */
#define ONE_POINT_2_IN_Q14 19661
/* 0.7 in Q12 */
#define O7_IN_Q12 2867
/* 0.2 in Q14 */
#define O2_IN_Q14 3277
/* 0.2 in Q15 */
#define O2_IN_Q15 6554

#endif /* ifndef TYPEDEF_H */
