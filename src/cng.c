/*
 cng.c
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

#include "cng.h"
#include "decodeAdaptativeCodeVector.h"
#include "decodeLSP.h"
#include "interpolateqLSP.h"
#include "qLSP2LP.h"
#include "g729FixedPointMath.h"
#include "codebooks.h"
#include "LP2LSPConversion.h"

/* buffers allocation */
static const word16_t SIDqLSPInitialValues[NB_LSP_COEFF] = {31441,  27566,  21458,  13612,   4663, -4663, -13612, -21458, -27566, -31441}; /* in Q0.15 the initials values for the previous qLSP buffer */

bcg729CNGChannelContextStruct *initBcg729CNGChannel() {
	/* create the context structure */
	bcg729CNGChannelContextStruct *CNGChannelContext = malloc(sizeof(bcg729CNGChannelContextStruct));
	memset(CNGChannelContext, 0, sizeof(*CNGChannelContext));

	memcpy(CNGChannelContext->qLSP, SIDqLSPInitialValues, NB_LSP_COEFF*sizeof(word16_t)); /* initialise the previousqLSP buffer */

	return CNGChannelContext;
}

/*******************************************************************************************/
/* computeComfortNoiseExcitationVector : as is spec B4.4 and B4.5                          */
/*    parameters:                                                                          */
/*      -(i): targetGain : the gain from ea B.19 in Q3                                     */
/*      -(i/o): randomGeneratorSeed : used to get a pseudo random number, is updated       */
/*      -(i/o): excitationVector in Q0, accessed in range [-L_PAST_EXCITATION,L_FRAME-1]   */
/*                                                        [-154,79]                        */
/*                                                                                         */
/*******************************************************************************************/
void computeComfortNoiseExcitationVector(word16_t targetGain, uint16_t *randomGeneratorSeed, word16_t *excitationVector) {
	int16_t fracPitchDelay,intPitchDelay;
	uint16_t randomNumberBuffer;
	uint8_t subframeIndex = 0;
	int i,j;
	/* shall we check targetGain is 0?? and set excitation vector[0,L_FRAME[ to 0 in this case?*/

	for (subframeIndex=0; subframeIndex<L_FRAME; subframeIndex+=L_SUBFRAME) { /* process 2 subframes */
		word16_t gaussianRandomExcitation[L_SUBFRAME];
		word32_t Eg = 0;
		word32_t Gg = 0; /* gain to be applied to the randomly generated gaussian to reach a Eg of 1/4*L_SUBFRAME*targetGain(note this doesn't appears in spec, only in ITU code)*/
		word16_t Ga = 0; /* gain to be applied to adaptative subframe excitation in Q0.15 */
		word32_t Ea = 0;
		word32_t Ei = 0; /* inter excitation : Sum[0..39] adaptativeExcitation*fixedExcitation, defined in eq B21*/
		word32_t K = 0;
		word32_t Gf = 0; /* gain to be applied to the fixed codebook excitation */
		word32_t x2 = 0; /* second root of the 2nd degre equation solved to get Gf */
		word16_t sign[4]; /* sign of the impulses of fixed codebook excitation */
		word16_t position[4]; /* positions of the impulses of fixed codebook excitation */
		uint8_t deltaScaleFactor=0;
		word64_t delta;

		/* generate pseudo random pitch delay in range [40,103] for adaptative codebook */
		randomNumberBuffer = pseudoRandom(randomGeneratorSeed); /* get 16 bits of pseudoRandom value*/
		fracPitchDelay = (int16_t)(randomNumberBuffer&0x0003)-1; /* fraction part of the pitch delay shall be -1, 0 or 1 */
		if (fracPitchDelay==2) fracPitchDelay = 0;
		randomNumberBuffer = randomNumberBuffer>>2; /* 14 random bits left */
		intPitchDelay = (randomNumberBuffer&0x003F) + 40; /* intPitchDelay in [40,103] */
		randomNumberBuffer = randomNumberBuffer>>6; /* 8 random bits left */
		/* generate pseudo random sign and position for fixed codebook */
		position[0] = (randomNumberBuffer&0x0007)*5;
		randomNumberBuffer = randomNumberBuffer>>3; /* 5 random bits left */
		sign[0] = randomNumberBuffer&0x0001;
		randomNumberBuffer = randomNumberBuffer>>1; /* 4 random bits left */
		position[1] = (randomNumberBuffer&0x0007)*5+1;
		randomNumberBuffer = randomNumberBuffer>>3; /* 1 random bits left */
		sign[1] = randomNumberBuffer&0x0001,
		randomNumberBuffer = pseudoRandom(randomGeneratorSeed); /* get 16 bits of pseudoRandom value*/
		position[2] = (randomNumberBuffer&0x0007)*5+2;
		randomNumberBuffer = randomNumberBuffer>>3; /* 13 random bits left */
		sign[2] = randomNumberBuffer&0x0001;
		randomNumberBuffer = randomNumberBuffer>>1; /* 12 random bits left */
		position[3] = (randomNumberBuffer&0x0001)+3; /*j+3*/
		randomNumberBuffer = randomNumberBuffer>>1; /* 11 random bits left */
		position[3] += (randomNumberBuffer&0x0007)*5;
		randomNumberBuffer = randomNumberBuffer>>3; /* 8 random bits left */
		sign[3] = randomNumberBuffer&0x0001;
		randomNumberBuffer = randomNumberBuffer>>1; /* 7 random bits left */
		/* randomly generate Ga : adaptative gain eqB.22: max is 0,5  */
		Ga = (pseudoRandom(randomGeneratorSeed)&0x1fff)<<1; /* get 16 bits of pseudoRandom value but make sure it is < 0.5 in Q15 */



		/* generate gaussian random excitation : get generation algo from ITU code, no reference to this in the spec... 
		 * Compute also the subframe energy(Sum [0..39] excitation^2) and then scale the sample to get an subframe average energy to targetGain/4 */
		for (i=0; i<L_SUBFRAME; i++) {
			word32_t tmpBuffer=0;
			for (j=0; j<12; j++) {
				tmpBuffer=ADD32(tmpBuffer, (word16_t)pseudoRandom(randomGeneratorSeed)); /* cast the unsigned 16 bits value to a signed one */
			}
			gaussianRandomExcitation[i]=(word16_t)(SHR(tmpBuffer,7));
			Eg = MAC16_16(Eg, gaussianRandomExcitation[i], gaussianRandomExcitation[i]);
		}

		/* compute coefficient = 1/2*sqrt(L_SUBFRAME/gaussianRandomExcitationSubframeEnergy)*targetGain (always>0) - retrieved from ITU code, no mention of this in the spec */
    		/* comments in code say 1/4*sqrt() but code actually implement 1/2... */
		Gg = MULT16_32_Q15(GAUSSIAN_EXCITATION_COEFF_FACTOR, g729InvSqrt_Q0Q31(Eg)); /* multiplicand in Q1.13 and Q0.31 -> result in Q1.29 */
		Gg = MULT16_32_Q15(targetGain, Gg); /* multiplicand in Q3 and Q1.29 -> result in Q17 */

		for (i=0; i<L_SUBFRAME; i++) {
			/* operate on positive value only to avoid problem when shifting to 0 a negative one */
			if (gaussianRandomExcitation[i]<0) {
				gaussianRandomExcitation[i] = -SATURATE(PSHR( MULT16_32_Q15(-gaussianRandomExcitation[i], Gg), 2), MAXINT16); /* gaussianRandom in Q0, targetGain in Q17 -> result in Q2, need to shift it by 2. */
			} else {
				gaussianRandomExcitation[i] = PSHR( MULT16_32_Q15(gaussianRandomExcitation[i], Gg), 2);
			}
		}
		/* generate random adaptative excitation and apply random gain Ga */
		computeAdaptativeCodebookVector(excitationVector+subframeIndex, fracPitchDelay, intPitchDelay);

		for (i=0; i<L_SUBFRAME; i++) {
			excitationVector[i+subframeIndex] = SATURATE(MULT16_16_P15(excitationVector[i+subframeIndex], Ga), MAXINT16);
		}


		/* add gaussian excitation to the adaptative one */
		for (i=0; i<L_SUBFRAME; i++) {
			excitationVector[i+subframeIndex] = SATURATE(ADD32(excitationVector[i+subframeIndex], gaussianRandomExcitation[i]),MAXINT16);
		}
	
		/* Compute Gf: fixed excitation gain salving equation B.21 in form 4*Gf^2 + 2I*Ga*Gf + Ea*Ga^2 - K = 0 (4Gf^2 + 2bGf + c = 0)*/
		/* compute Ea*Ga^2 */
		for (i=0; i<L_SUBFRAME; i++) {
			Ea = MAC16_16(Ea, excitationVector[i+subframeIndex], excitationVector[i+subframeIndex]); /* Ea actually contains Ea*Ga^2 as we already applied gain Ga on adaptative excitation, in Q0 */
		}

		/* compute Ei = I*Ga : fixed codebook hasn't been scaled yet, so it just 4 signed pulses of height 1, just get these one in our computation, in Q0 */
		Ei = 0;
		for (i=0; i<4; i++) {
			if (sign[i] == 0) { /* negative impulse */
				Ei = SUB32(Ei,excitationVector[position[i]+subframeIndex]);
			} else { /* positive impulse */
				Ei = ADD32(Ei,excitationVector[position[i]+subframeIndex]);
			}
		}

		/* compute K = L_SUBRAME*targetGain in Q3 */
		K = MULT16_32(targetGain, SHR(MULT16_16(L_SUBFRAME, targetGain), 3));
		
		/* compute delta = b^2 -ac = Ei^2 + 4*(K-Ea) in Q0 */
		delta = ADD64(MULT32_32(Ei,Ei), SHR(SUB64(K,SHL64((word64_t)Ea,3)),1));

		if (delta<0) {
			/* cancel adaptative excitation, keep gaussian one only */
			for (i=0; i<L_SUBFRAME; i++) {
				excitationVector[i+subframeIndex] = gaussianRandomExcitation[i];
			}

			/* compute again Ei and delta = Ei^2 + 3/4*K (from ITU code, no idea why) */
			for (i=0; i<4; i++) {
				if (sign[i] == 0) { /* negative impulse */
					Ei = SUB32(Ei,excitationVector[position[i]+subframeIndex]);
				} else { /* positive impulse */
					Ei = ADD32(Ei,excitationVector[position[i]+subframeIndex]);
				}
			}
			delta = ADD64(MULT32_32(Ei,Ei), MULT16_32_P15(COEFF_K,K)); /* COEFF_K is 0.75 in Q15 */
		}

		/* scale delta to have it fitting on 32 bits */
		deltaScaleFactor = 0;
		while (delta>=0x0000000080000000) {
			delta = delta>>1;
			deltaScaleFactor++;
		}
		/* deltaScaleFactor must be even */
		if (deltaScaleFactor%2==1) {
			delta = delta >> 1;
			deltaScaleFactor++;
		}

		delta =  g729Sqrt_Q0Q7((word32_t)delta); /* delta in Q(7-deltaScaleFactor/2)*/

		/* scale b (Ei) to the same scale */
		Ei = VSHR32(Ei, deltaScaleFactor/2-7);

		/* compute the two roots and pick the one with smaller absolute value */
		/* roots are (-b +-sqrt(delta))/a. We always have a=4, divide by four when rescaling from Q(7-deltaScaleFactor) to Q0 the final result */
		Gf = SUB32(delta, Ei); /* x1 = -b + sqrt(delta) in Q(7-deltaScaleFactor) */
		x2 = -ADD32(delta, Ei); /* x2 = -b - sqrt(delta) in Q(7-deltaScaleFactor) */
		if (ABS(x2)<ABS(Gf)) Gf = x2; /* pick the smallest (in absolute value of the roots) */
		Gf = VSHR32(Gf, 2+7-deltaScaleFactor/2); /* scale back Gf in Q0 and divide by 4 (+2 in the right shift) to get the actual root */

		/* add fixed codebook excitation */
		for (i=0; i<4; i++) {
			if (sign[i] == 0) { /* negative impulse */
				excitationVector[position[i]+subframeIndex] = SUB32(excitationVector[position[i]+subframeIndex], Gf);
			} else { /* positive impulse */
				excitationVector[position[i]+subframeIndex] = ADD32(excitationVector[position[i]+subframeIndex], Gf);
			}
		}
	}
}

/*******************************************************************************************/
/* decodeSIDframe : as is spec B4.4 and B4.5                                               */
/*    first check if we have a SID frame or a missing/untransmitted frame                  */
/*    for SID frame get paremeters(gain and LSP)                                           */
/*    Then generate excitation vector and update qLSP                                      */
/*    parameters:                                                                          */
/*      -(i/o):CNGChannelContext : context containing all informations needed for CNG      */
/*      -(i): previousFrameIsActiveFlag: true if last decoded frame was an active one      */
/*      -(i): bitStream: for SID frame contains received params as in spec B4.3,           */
/*                       NULL for missing/untransmitted frame                              */
/*      -(i): bitStreamLength : in bytes, length of previous buffer                        */
/*      -(i/o): excitationVector in Q0, accessed in range [-L_PAST_EXCITATION,L_FRAME-1]   */
/*                                                        [-154,79]                        */
/*      -(o): LP: 20 LP coefficients in Q12                                                */
/*      -(i/o): previousqLSP : previous quantised LSP in Q0.15 (NB_LSP_COEFF values)       */
/*      -(i/o): pseudoRandomSeed : seed used in the pseudo random number generator         */
/*      -(i/o): previousLCodeWord: in Q2.13, buffer to store the last 4 frames codewords,  */
/*              used to compute the current qLSF                                           */
/*                                                                                         */
/*******************************************************************************************/
void decodeSIDframe(bcg729CNGChannelContextStruct *CNGChannelContext, uint8_t previousFrameIsActiveFlag, uint8_t *bitStream, uint8_t bitStreamLength, word16_t *excitationVector, word16_t *previousqLSP, word16_t *LP, uint16_t *pseudoRandomSeed, word16_t previousLCodeWord[MA_MAX_K][NB_LSP_COEFF], uint8_t rfc3389PayloadFlag) {
	int i;
	word16_t interpolatedqLSP[NB_LSP_COEFF]; /* interpolated qLSP in Q0.15 */
	/* if this is a SID frame, decode received parameters */
	if (bitStream!=NULL) {
		if (rfc3389PayloadFlag) {
			int j;
			word32_t LPCoefficients[NB_LSP_COEFF+1]; /* in Q4.27 */
			word16_t LPCoefficientsQ12[NB_LSP_COEFF]; /* in Q12 */
			word32_t previousIterationLPCoefficients[NB_LSP_COEFF+1]; /* in Q4.27 */
			uint8_t CNFilterOrder = (bitStreamLength-1); /* first byte is noise energy level */
			word16_t k[NB_LSP_COEFF];
			word32_t receivedSIDGainLog;

			if (CNFilterOrder>NB_LSP_COEFF) { /* if rfc3389 payload have a filter order > than supported, just ignore the last coefficients */
				CNFilterOrder = NB_LSP_COEFF;
			}
			/* retrieve gain from codebook according to Gain parameter */
			//CNGChannelContext->receivedSIDGain = SIDGainCodebook[bitStream[0]];
			/* received parameter is -(10*log10(meanE) -90) so retrieve mean energy : 10^((param+90)/10) */
			/* but expected parameter is a gain applied to single sample so sqrt of previous result */
			receivedSIDGainLog = ADD32(-bitStream[0], 90);
			if (receivedSIDGainLog > 66) {
				receivedSIDGainLog = 66; /* noise level shall not be too high in any case */
			} /* receivedSIDGainLog is param+90 in Q0 */
			receivedSIDGainLog = MULT16_16(receivedSIDGainLog, 680); /* 680 is 1/(10*log10(2)) in Q11 -> receivedSIDGainLog is ((param+90)/10)/log10(2) = log2(meanE) in Q11 */
			receivedSIDGainLog = g729Exp2_Q11Q16(receivedSIDGainLog); /* receivedSIDGainLog in meanE in Q16 */
			if (receivedSIDGainLog > 0 ) { /* avoid arithmetic problem if energy is too low */
				CNGChannelContext->receivedSIDGain = (word16_t)(SHR(g729Sqrt_Q0Q7(receivedSIDGainLog), 12)); /* output of sqrt in Q15, result needed in Q3  */
				if (CNGChannelContext->receivedSIDGain < SIDGainCodebook[0]) {
					CNGChannelContext->receivedSIDGain = SIDGainCodebook[0];
				}
			} else { /* minimum target gain extracted from codebook */
				CNGChannelContext->receivedSIDGain = SIDGainCodebook[0];
			}

			/* retrieve LP from rfc3389 payload*/
			/* rebuild the LP coefficients using algo given in G711 Appendix II section 5.2.1.3 */
			/* first get back the quantized reflection coefficients from Index as in RFC3389 3.2 */
			for (i=0; i<CNFilterOrder; i++) {
				k[i] = MULT16_16(ADD16(bitStream[i+1],127), 258); /* k in Q15 */
			}
			for (i=CNFilterOrder; i<NB_LSP_COEFF; i++) { /* rfc3389 payload may provide a filter of order < NB_LSP_COEFF */
				k[i] = 0; /* in that case, just set the last coefficients to 0 */
			}

			/* rebuild LP coefficients */
			LPCoefficients[0] = ONE_IN_Q27;
			LPCoefficients[1] = -SHL(k[0],12);
			for (i=2; i<NB_LSP_COEFF+1; i++) {
				/* update the previousIterationLPCoefficients needed for this one */
				for (j=1; j<i; j++) {
					previousIterationLPCoefficients[j] = LPCoefficients[j];
				}

				LPCoefficients[i] = -SHL(k[i-1],16); /* current coeff in Q31 while older one in Q27*/
				for (j=1; j<i; j++) {
					LPCoefficients[j] = MAC32_32_Q31(LPCoefficients[j], LPCoefficients[i], previousIterationLPCoefficients[i-j]); /*LPCoefficients in Q27 except for LPCoefficients[i] in Q31 */
				}
				LPCoefficients[i] = SHR(LPCoefficients[i], 4);
			}

			/* convert with rounding the LP Coefficients form Q27 to Q12, ignore first coefficient which is always 1 */
			for (i=0; i<NB_LSP_COEFF; i++) {
				LPCoefficientsQ12[i] = (word16_t)SATURATE(PSHR(LPCoefficients[i+1], 15), MAXINT16);
			}

			/* compute LSP from LP as we need to store LSP in context as the LP to use are interpolated with previous ones. Note: use LSP as qLSP */
			if (!LP2LSPConversion(LPCoefficientsQ12, CNGChannelContext->qLSP)) {
				/* unable to find the 10 roots repeat previous LSP */
				memcpy(CNGChannelContext->qLSP, previousqLSP, NB_LSP_COEFF*sizeof(word16_t));
			}
		} else { /* regular G729 SID payload on 2 bytes */
			word16_t currentqLSF[NB_LSP_COEFF]; /* buffer to the current qLSF in Q2.13 */
			uint8_t L0 = (bitStream[0]>>7)&0x01;
			uint8_t L1Index = (bitStream[0]>>2)&0x1F;
			uint8_t L2Index = ((bitStream[0]&0x03)<<2) | ((bitStream[1]>>6)&0x03);

			/* retrieve gain from codebook according to Gain parameter */
			CNGChannelContext->receivedSIDGain = SIDGainCodebook[((bitStream[1])>>1)&0x1F];
			/* Use L1 and L2(parameter for first and second stage vector of LSF quantizer) to retrieve LSP using subset index to address the complete L1 and L2L3 codebook */
			for (i=0; i<NB_LSP_COEFF/2; i++) {
				currentqLSF[i]=ADD16(L1[L1SubsetIndex[L1Index]][i], L2L3[L2SubsetIndex[L2Index]][i]);
			}
			for (i=NB_LSP_COEFF/2; i<NB_LSP_COEFF; i++) {
				currentqLSF[i]=ADD16(L1[L1SubsetIndex[L1Index]][i], L2L3[L3SubsetIndex[L2Index]][i]);
			}
			computeqLSF(currentqLSF, previousLCodeWord, L0, noiseMAPredictor, noiseMAPredictorSum);

			/* convert qLSF to qLSP: qLSP = cos(qLSF) */
			for (i=0; i<NB_LSP_COEFF; i++) {
				CNGChannelContext->qLSP[i] = g729Cos_Q13Q15(currentqLSF[i]); /* ouput in Q0.15 */
			}
		}
	} /* Note: Itu implementation have information to sort missing and untransmitted packets and perform reconstruction of missed SID packet when it detects it, we cannot differentiate lost vs untransmitted packet so we don't do it */

	/* compute the LP coefficients */
	interpolateqLSP(previousqLSP, CNGChannelContext->qLSP, interpolatedqLSP);
	/* copy the current qLSP(SID ones) to previousqLSP buffer */
	for (i=0; i<NB_LSP_COEFF; i++) {
		previousqLSP[i] = CNGChannelContext->qLSP[i];
	}

	/* call the qLSP2LP function for first subframe */
	qLSP2LP(interpolatedqLSP, LP);
	/* call the qLSP2LP function for second subframe */
	qLSP2LP(CNGChannelContext->qLSP, &(LP[NB_LSP_COEFF]));


	/* apply target gain smoothing eq B.19 */
	if (previousFrameIsActiveFlag) {
		/* this is the first SID frame use transmitted(or recomputed from last valid frame energy) SID gain */
		CNGChannelContext->smoothedSIDGain = CNGChannelContext->receivedSIDGain;
	} else { /* otherwise 7/8 of last smoothed gain and 1/8 of last received gain */
		CNGChannelContext->smoothedSIDGain = SUB16(CNGChannelContext->smoothedSIDGain, (CNGChannelContext->smoothedSIDGain>>3));
		CNGChannelContext->smoothedSIDGain = ADD16(CNGChannelContext->smoothedSIDGain, (CNGChannelContext->receivedSIDGain>>3));
	}

	/* get the excitation vector */
	computeComfortNoiseExcitationVector(CNGChannelContext->smoothedSIDGain, pseudoRandomSeed, excitationVector);
}
