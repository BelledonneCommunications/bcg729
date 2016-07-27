/*
 vad.c
 Voice Activity Detection

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
#include "g729FixedPointMath.h"
#include "codebooks.h"

#include "vad.h"

#define VOICE 1
#define NOISE 0

bcg729VADChannelContextStruct *initBcg729VADChannel() {
	int i;
	/* create the context structure */
	bcg729VADChannelContextStruct *VADChannelContext = malloc(sizeof(bcg729VADChannelContextStruct));
	memset(VADChannelContext, 0, sizeof(*VADChannelContext)); /* set meanLSF buffer to 0 */
	VADChannelContext->frameCount = 0;
	VADChannelContext->updateCount = 0;
	for (i=0; i<N0; i++) {
		VADChannelContext->EfBuffer[i] = MAXINT16;
	}
	VADChannelContext->SVDm1 = VOICE;
	VADChannelContext->SVDm2 = VOICE;
	VADChannelContext->Count_inert = 0;
	VADChannelContext->secondStageVADSmoothingFlag = 1;
	VADChannelContext->smoothingCounter = 0;
	VADChannelContext->previousFrameEf = 0;
	VADChannelContext->noiseContinuityCounter = 0;

	return VADChannelContext;
}

/*******************************************************************************************/
/* multiBoundaryInitialVoiceActivityDecision as described in B3.5                          */
/*    parameters:                                                                          */
/*      -(i) deltaS  spectral distorsion as decribed in B3.4.1 in Q13                      */
/*      -(i) deltaEf full band energy difference as described in B3.4.2 in Q11             */
/*      -(i) deltaEl low band energy difference as described in B3.4.3 in Q11              */
/*      -(i) deltaZC Zero Crossing difference as described in B3.4.4 in Q15                */
/*    returns : Ivd as specified in B3.5 : VOICE or NOISE                                  */
/*                                                                                         */
/*******************************************************************************************/
static uint8_t multiBoundaryInitialVoiceActivityDecision(word32_t deltaS, word16_t deltaEf, word16_t deltaEl, word16_t deltaZC) {
	//word32_t acc1, acc2;

	/* test the 14 conditions of B3.5 */
	/* Note : the constant used in itu code are not the one in spec. Code seems erratic with comment irrelevant, use constant found in floating point version, annex C+ */
	/* static FLOAT a[14] = { 1.750000e-03, -4.545455e-03, -2.500000e+01,
	 * 			( 2.000000e+01, 0.000000e+00, 8.800000e+03,
	 * 			0.000000e+0VADChannelContext->0, 2.5e+01, -2.909091e+01,
	 * 			0.000000e+00, 1.400000e+04, 0.928571,
	 * 			-1.500000e+00, 0.714285}; 
	 *
	 * static FLOAT b[14] = { 0.00085, 0.001159091, -5.0,
	 * 			-6.0, -4.7, -12.2,
	 * 			0.0009, -7.0, -4.8182,
	 * 			-5.3, -15.5, 1.14285,
	 * 			-9.0, -2.1428571};
	 * These constants are for Ef and El computed correctlVADChannelContext->y, we have Ef/10 and El/10 in buffers, and lsf normalised (/2pi)-> deltaS/4pi^2
	*/
	/* adjust input parameters */
	word32_t deltaEf32 = MULT16_16(10, deltaEf); /* in Q11 */
	word32_t deltaEl32 = MULT16_16(10, deltaEl); /* in Q11 */
	deltaS = MULT16_32(830, deltaS); /* 830 is 1/(4*pi^2) in Q15, deltaS in Q28 now */

	/* cond 1 : deltaS > a1*deltaZC + b1 */
	if (deltaS > ADD32(MULT16_32_Q12(deltaZC, 58720), 228170)) { /* deltaS in Q28 - deltaZC in Q15*58720(1.75e-3 in Q25) >> 12 -> result in Q28 + 228170 (0.00085 in Q28) */
		return VOICE;
	}

	/* cond 2 : deltaS > a2*deltaZC + b2 */
	if (deltaS > ADD32(MULT16_32_Q12(deltaZC, -152520), 311141)) { /* deltaS in Q28 deltaZC in Q15*-152520(-4.545455e-03 in Q25) >> 12 -> result in Q28 + 311141 (0.001159091 in Q28) */
		return VOICE;
	}

	/* cond 3 : deltaEf < a3*deltaZC + b3 */
	if (deltaEf32 < ADD32(MULT16_32_Q15(deltaZC, -51200), -10240)) { /* deltaEf32 in Q11 - deltaZC in Q15*-51200(-2.50e+01 in Q11) >> 15 -> result in Q11 - 10240 (-5 in Q11) */
		return VOICE;
	}

	/* cond 4 : deltaEf < a4*deltaZC + b4 */
	if (deltaEf32 < ADD32(MULT16_32_Q15(deltaZC, 40960), -12288)) { /* deltaEf32 in Q11 - deltaZC in Q15*40960(2.0e+01 in Q11) >> 15 -> result in Q11 - 12288 (-6.0 in Q11) */
		return VOICE;
	}
	
	/* cond 5 : deltaEf < b5 */
	if (deltaEf32 < -9626) { /* deltaEf32 in Q11 - -4.7 in Q11 -> 9626 */
		return VOICE;
	}
	
	/* cond 6 : deltaEf < a6*deltaS + b6 */
	if (deltaEf32 < ADD32(MULT16_32_Q12(275,deltaS), -24986)) { /* deltaEf32 in Q11 - deltaS in Q28*275(8.8e+03 in Q-5) >> 12 -> result in Q11 - -24986 (-12.2 in Q11) */
		return VOICE;
	}
	
	/* cond 7 : deltaS > b7 */
	if (deltaS > 241592) { /* deltaS in Q28 - 0.0009 in Q28 -> 241592 */
		return VOICE;
	}
	
	/* cond 8 : deltaEf < a8*deltaZC + b8 */
	if (deltaEf32 < ADD32(MULT16_32_Q15(deltaZC, 51200), -14336)) { /* deltaEf32 in Q11 - deltaZC in Q15*51200(2.50e+01 in Q11) >> 15 -> result in Q11 - -14336 (-7 in Q11) */
		return VOICE;
	}

	/* cond 9 : deltaEf < a9*deltaZC + b9 */
	if (deltaEf32 < ADD32(MULT16_32_Q15(deltaZC, -59578), -9868)) { /* deltaEf32 in Q11 - deltaZC in Q15*59578(-2.909091e+01 in Q11) >> 15 -> result in Q11 - -9868 (-4.8182 in Q11) */
		return VOICE;
	}

	/* cond 10 : deltaEf < b10 */
	/* Note: cond 10 is totally useless : already covered by cond 5 before - skip it.*/
	if (deltaEf32 < -10854) { /* deltaEf32 in Q11 - -5.3 in Q11 -> 10854 */
		return VOICE;
	}
	
	/* cond 11 : deltaEl < a11*deltaS + b11 */
	if (deltaEl32 < ADD32(MULT16_32_Q13(875,deltaS), -31744)) { /* deltaEl32 in Q11 - deltaS in Q28*875(1.4000e+04 in Q-4) >> 12 -> result in Q11 - -31744 (-15.5 in Q11) */
		return VOICE;
	}
	
	/* cond 12 : deltaEl > a12*deltaEf + b12 */
	if (deltaEl32 > ADD32(MULT16_32_Q15(30427, deltaEl32), 2341)) { /* deltaEl32 in Q11 - deltaEl32 in Q11*30427(0.928571 in Q15) >> 15 -> result in Q11 - 2341 (1.14285 in Q11) */
		return VOICE;
	}

	/* cond 13 : deltaEl < a13*deltaEf + b13 */
	if (deltaEl32 < ADD32(MULT16_32_Q14(-24576, deltaEl32), -18432)) { /* deltaEl32 in Q11 - deltaEl32 in Q11*-24576(-1.5 in Q14) >> 15 -> result in Q11 - -18432 (-9 in Q11) */
		return VOICE;
	}

	/* cond 13 : deltaEl < a14*deltaEf + b14 */
	if (deltaEl32 < ADD32(MULT16_32_Q15(23406, deltaEl32), -4389)) { /* deltaEl32 in Q11 - deltaEl32 in Q11*23406(0.714285 in Q15) >> 15 -> result in Q11 - -4389 (-2.1428571 in Q11) */
		return VOICE;
	}
	
	/* no condition was true */
	return NOISE;
}

/*******************************************************************************************/
/* bcg729_vad : voice activity detection from AnnexB                                       */
/*    parameters:                                                                          */
/*      -(i/o):VADChannelContext : context containing all informations needed for VAD      */
/*      -(i): reflectionCoefficient : in Q31                                               */
/*      -(i): LSFCoefficients :10 Coefficient in Q2.13 in range [0,Pi[                     */
/*      -(i): autoCorrelationCoefficients : 13 coefficients of autoCorrelation             */
/*      -(i): autoCorrelationCoefficientsScale : scaling factor of previous coeffs         */
/*      -(i): signalCurrentFrame: the preprocessed signal in Q0, accessed in [-1, L_FRAME[ */
/*                                                                                         */
/*    returns : 1 for active voice frame, 0 for silence frame                              */
/*                                                                                         */
/*******************************************************************************************/
uint8_t bcg729_vad(bcg729VADChannelContextStruct *VADChannelContext, word32_t reflectionCoefficient, word16_t *LSFCoefficients, word32_t *autoCorrelationCoefficients, int8_t autoCorrelationCoefficientsScale, const word16_t *signalCurrentFrame) {

	word16_t Ef, deltaEf; /* full band energy/10 in Q11 eq B1 */
	word16_t Emin; /* minimum Ef value of the last 128 frames */
	word16_t El, deltaEl; /* low band energy/10 in Q11 eqB2 */
	word16_t ZC, deltaZC; /* zero crossing rate in Q15 eq B3 */
	word32_t deltaS; /* spectral distorsion in Q12 */
	word32_t acc;
	uint8_t Ivd = VOICE; /* Initial Voice activity decision : VOICE or NOISE, flag defined in B3.5 */

	int i;
	
	/*** parameters extraction B3.1 ***/
	/* full band energy Ef (eq B1) = 10*log10(autoCorrelationCoefficient[0]/240), we compute Ef/10 in Q11 */
	acc = SUB32(g729Log2_Q0Q16(autoCorrelationCoefficients[0]), (((int32_t)autoCorrelationCoefficientsScale)<<16)); /* acc = log2(R(0)) in Q16*/
	acc = SHR32(SUB32(acc, LOG2_240_Q16),1); /* acc = log2(R(0)/240) in Q15 */
	acc = MULT16_32_Q15(INV_LOG2_10_Q15, acc); /* acc Ef in Q15 */
	Ef = (word16_t)(PSHR(acc,4));

	/* store Ef in the rolling EfBuffer at position countFrame%N0 */
	VADChannelContext->EfBuffer[(VADChannelContext->frameCount)%N0] = Ef;

	/* low band energy El (eq B2)/10 in Q11 */
	acc=MULT16_32_Q15(lowBandFilter[0], autoCorrelationCoefficients[0]);
	for (i=1; i<NB_LSP_COEFF+3; i++) {
		acc = MAC16_32_Q14(acc, lowBandFilter[i], autoCorrelationCoefficients[i]);
	}
	acc = SUB32(g729Log2_Q0Q16(acc), (((int32_t)autoCorrelationCoefficientsScale)<<16)); /* acc = log2(htRh) in Q16*/
	acc = SHR32(SUB32(acc, LOG2_240_Q16),1); /* acc = log2(htRh/240) in Q15 */
	acc = MULT16_32_Q15(INV_LOG2_10_Q15, acc); /* acc El in Q15 */
	El = (word16_t)(PSHR(acc,4));

	/* compute zero crossing rate ZC (eq B3) in Q15 : at each zero crossing, add 410 (1/80 in Q15)*/
	ZC = 0;
	for (i=0; i<L_FRAME; i++) {
		if (MULT16_16(signalCurrentFrame[i-1], signalCurrentFrame[i])<0) {
			ZC = ADD16(ZC,410); /* add 1/80 in Q15 */
		}
	}
	
	/* B3.2 Initialisation of the running averages of background noise characteristics : End of init period (32 first frames) */
	if (VADChannelContext->frameCount == Ni) { /* complete initialisations */
		word16_t meanEn = 0;
		
		if (VADChannelContext->nbValidInitFrame>0) {/* check if we had a valid frame during init */
			meanEn = (word16_t)DIV32(VADChannelContext->initEfSum, VADChannelContext->nbValidInitFrame);
			VADChannelContext->meanZC = (word16_t)DIV32(VADChannelContext->initZCSum, VADChannelContext->nbValidInitFrame);
			for (i=0; i<NB_LSP_COEFF; i++) {
				VADChannelContext->meanLSF[i] = (word16_t)DIV32(VADChannelContext->initLSFSum[i], VADChannelContext->nbValidInitFrame);
			}
			/* spec B3.2 describe a mechanism for initialisation of meanEl and meanEf which is not implemented in ITU code */
			/* ITU code does: meanEf = meanEn-1 and meanEl = meanEn - 1.2, just do that */
			/* it looks like the case in wich meanEn>T2 (5.5dB) is considered always true which is certainly not the case */
			VADChannelContext->meanEf = SUB16(meanEn, 2048); /* -1 in Q11 (K4 if K4 is in Q27 in the spec)*/
			VADChannelContext->meanEl = SUB16(meanEn, 2458); /* -1.2 in Q11 (K5 if K5 is in Q27 in the spec)*/
		} else {/* if we have no valid frames */
			VADChannelContext->frameCount = 0; /* as done in Appendix II : reset frameCount to restart init period */
		}
	}

	/* B3.2 Initialisation of the running averages of background noise characteristics : during init period(32 first frames) */
	if (VADChannelContext->frameCount < Ni) { /* initialisation is still on going for running averages */
		if (Ef<3072) { /* Ef stores 1/10 of full band energy in Q11, frames are valid only when Ef>15dB, 3072 is 1.5 in Q11 */
			Ivd = NOISE;
		} else {
			Ivd = VOICE;
			VADChannelContext->nbValidInitFrame++;
			VADChannelContext->initEfSum = ADD32(VADChannelContext->initEfSum, Ef);
			VADChannelContext->initZCSum = ADD32(VADChannelContext->initZCSum, ZC);
			for (i=0; i<NB_LSP_COEFF; i++) {
				VADChannelContext->initLSFSum[i] = ADD32(VADChannelContext->initLSFSum[i], LSFCoefficients[i]);
			}
		}

		/* context updates */
		VADChannelContext->frameCount++;
		VADChannelContext->previousFrameEf = Ef;
		VADChannelContext -> SVDm2 = VADChannelContext -> SVDm1;
		VADChannelContext -> SVDm1 = Ivd;

		return Ivd;
	}

	/* B3.3 Generating the long term minimum energy (real computation of the minimum over the last 128 values, no trick as suggested in the spec) */
	Emin = getMinInArray(VADChannelContext->EfBuffer, N0);

	/* B3.4.1 spectral distorsion deltaS */
	deltaS = 0;
	for (i=0; i<NB_LSP_COEFF; i++) {
		word16_t acc16;
		acc16 = SUB16(LSFCoefficients[i], VADChannelContext->meanLSF[i]);
		deltaS = MAC16_16_Q13(deltaS, acc16, acc16);
	}

	/* B3.4.2 deltaEf */
	deltaEf = SUB16(VADChannelContext->meanEf, Ef); /* in Q11 */
	
	/* B3.4.3 deltaEf */
	deltaEl = SUB16(VADChannelContext->meanEl, El); /* in Q11 */
	
	/* B3.4.2 deltaEf */
	deltaZC = SUB16(VADChannelContext->meanZC, ZC); /* in Q15 */
	
	if (Ef<3072) { /* Ef stores 1/10 of full band energy in Q11, frames are valid only when Ef>15dB, 3072 is 1.5 in Q11 */
		Ivd = NOISE;
	} else {
		Ivd = multiBoundaryInitialVoiceActivityDecision(deltaS, deltaEf, deltaEl, deltaZC);
	}
	
	/* B3.6 : Voice Activity Decision Smoothing */
	/* Appendix II hysteresis II 5.2 as implemented in ITU code, description in appendix has a test on meanEf which is not implemented in example code */
	if (Ivd == VOICE) {
		VADChannelContext->Count_inert=0;
	}

	if ((Ivd == NOISE) &&  (VADChannelContext->Count_inert < 6)) {
		VADChannelContext->Count_inert++;
		Ivd = VOICE;
	}

	/* first stage of VAD smoothing : code add a test on Ef>15 (21 in the floating version?!?) not documented */
	if ((Ivd == NOISE) && (VADChannelContext->SVDm1) && (deltaEf>410) && (Ef>3072)) {  /* deltaEf is (meanEf - Ef)/10 in Q11, test is Ef > meanEf + 2, 410 is 0.2 in Q11 - 3072 is 1.5 in Q11 */
		Ivd = VOICE;
	}

	/* second stage of VAD smoothing */
	if ((VADChannelContext->secondStageVADSmoothingFlag == 1)
	&& (Ivd == NOISE)
	&& (VADChannelContext->SVDm1 == VOICE)
	&& (VADChannelContext->SVDm2 == VOICE)
	&& (ABS(SUB16(Ef, VADChannelContext->previousFrameEf)) <= 614)) { /* |Ef - meanEf|<=3 -> 614 is 0.3 in Q11 */
		Ivd = VOICE;
		VADChannelContext->smoothingCounter++;
		if (VADChannelContext->smoothingCounter<=4) {
			VADChannelContext->secondStageVADSmoothingFlag = 1;
		} else {
			VADChannelContext->secondStageVADSmoothingFlag = 0;
			VADChannelContext->smoothingCounter = 0;
		}

	} else {
		VADChannelContext->secondStageVADSmoothingFlag = 1;
	}
	
	/* third stage of VAD smoothing */
	if (Ivd == NOISE) {
		VADChannelContext->noiseContinuityCounter++;
	}

	if ( (Ivd == VOICE)
	&& (VADChannelContext->noiseContinuityCounter > 10) /* Cs > N2 */
	&& (SUB16(Ef, VADChannelContext->previousFrameEf) <= 614)) { /* Ef - prevEf <= T5(3.0) : 614 is 0.3 in Q11 */
		Ivd = NOISE;
		VADChannelContext->noiseContinuityCounter = 0;
		VADChannelContext->Count_inert = 6; /* added by appendix II*/
	}

	if (Ivd == VOICE) {
		VADChannelContext->noiseContinuityCounter = 0;
	}

	/* forth stage of VAD smoothing is not implemented in appendix II */

	/* update the running averages B3.7 : note ITU implementation differs from spec, not only Ef < meanEf + 3dB is tested but also ReflectionCoefficient < 0.75  and deltaS < 0.002532959 */
	if ( (SUB16(Ef, 614) < VADChannelContext->meanEf) /* 614 is 0.3 in Q11 : Ef and meanEf are 1/10 of real Ef and meanEf, so 0.3 instead of 3, all in Q11 */
	    && (reflectionCoefficient < 1610612736) ) { /* in Q31, 0.75 is 1610612736 */
		/* Note : G729 Appendix II suggest that the test on deltaS is not relevant and prevent good detection in very noisy environment : remove it */
	//    && (deltaS < 819)) { /* deltaS is computed using non normalised LSF so compare with 4*pi^2*0.002532959 = 819 in Q13 as ITU code normalise LSF dividing them by 2*Pi */
		/* all beta Coefficient in Q15 */
		word16_t betaE, betaZC, betaLSF; /* parameter used for first order auto-regressive update scheme, note betaEf and betaEl are the same: betaE */
		word16_t betaEComplement, betaZCComplement, betaLSFComplement; /* these are the (1 - betaXX) */
		/* update number of update performed (Cn in spec B3.7) */
		VADChannelContext->updateCount++;

		/* according to spec, based on Cn (updateCount) value we must use a different set of coefficient, none of them is defined in the spec, all values are picked from ITU code */
		if (VADChannelContext->updateCount<20) {
			betaE = 24576; /* 0.75 */
			betaEComplement = 8192; /* 1-0.75 */
			betaZC = 26214; /* 0.8 */
			betaZCComplement = 6554; /* 1-0.8 */
			betaLSF = 19661; /* 0.6 */
			betaLSFComplement = 13107; /* 1-0.6 */
		} else if (VADChannelContext->updateCount<30) {
			betaE = 31130; /* 0.95 */
			betaEComplement = 1638; /* 1-0.95 */
			betaZC = 30147; /* 0.92 */
			betaZCComplement = 2621; /* 1-0.92 */
			betaLSF = 21299; /* 0.65 */
			betaLSFComplement = 11469; /* 1-0.65 */
		} else if (VADChannelContext->updateCount<40) {
			betaE = 31785; /* 0.97 */
			betaEComplement = 983; /* 1-0.97 */
			betaZC = 30802; /* 0.94 */
			betaZCComplement = 1966; /* 1-0.94 */
			betaLSF = 22938; /* 0.7 */
			betaLSFComplement = 9830 ; /* 1-0.7 */
		} else if (VADChannelContext->updateCount<50) {
			betaE = 32440; /* 0.99 */
			betaEComplement = 328; /* 1-0.99 */
			betaZC = 31457; /* 0.96 */
			betaZCComplement = 1311; /* 1-0.96 */
			betaLSF = 24756; /* 0.75 */
			betaLSFComplement = 8192; /* 1-0.75 */
		} else if (VADChannelContext->updateCount<60) {
			betaE = 32604; /* 0.995 */
			betaEComplement = 164; /* 1-0.995 */
			betaZC = 32440; /* 0.99 */
			betaZCComplement = 328; /* 1-0.99 */
			betaLSF = 24576; /* 0.75 */
			betaLSFComplement = 8192; /* 1-0.75 */
		} else {
			betaE = 32702; /* 0.998 */ /* note : in ITU code betaE and betaZC seems to be swaped on this last case, this implementation swap them back */
			betaEComplement = 66; /* 1-0.998 */
			betaZC = 32604; /* 0.995 */
			betaZCComplement = 164; /* 1-0.995 */
			betaLSF = 24576; /* 0.75 */
			betaLSFComplement = 8192; /* 1-0.75 */
		}

		VADChannelContext->meanEf = ADD16(MULT16_16_Q15(VADChannelContext->meanEf, betaE), MULT16_16_Q15(Ef, betaEComplement));
		VADChannelContext->meanEl = ADD16(MULT16_16_Q15(VADChannelContext->meanEl, betaE), MULT16_16_Q15(El, betaEComplement));
		VADChannelContext->meanZC = ADD16(MULT16_16_Q15(VADChannelContext->meanZC, betaZC), MULT16_16_Q15(ZC, betaZCComplement));
		for (i=0; i<NB_LSP_COEFF; i++) {
			VADChannelContext->meanLSF[i] = ADD16(MULT16_16_Q15(VADChannelContext->meanLSF[i], betaLSF), MULT16_16_Q15(LSFCoefficients[i], betaLSFComplement));
		}
	}

	/* further update of meanEf and updatedFrame, implementation differs from spec again, stick to ITU code */
	if ( (VADChannelContext->frameCount>N0)
		&&( 	((VADChannelContext->meanEf<Emin) && (deltaS < 819)) /* deltaS < 0.002532959 -> 819 is 0.002532959*4*pi^2 in Q13 */
			||(VADChannelContext->meanEf > ADD16(Emin, 2048)))) { /* meanEf > Emin + 10 : 2048 is 1 in Q11, Ef is 1/10 of real one */

		VADChannelContext->meanEf = Emin;
		VADChannelContext->updateCount = 0;
	}

	/* context updates */
	VADChannelContext->frameCount++;
	VADChannelContext->previousFrameEf = Ef;
	VADChannelContext -> SVDm2 = VADChannelContext -> SVDm1;
	VADChannelContext -> SVDm1 = Ivd;

	return Ivd;
}
