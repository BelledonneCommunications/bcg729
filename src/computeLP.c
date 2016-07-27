/*
 computeLP.c

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
#include "codebooks.h"
#include "utils.h"

#include "computeLP.h"

/*****************************************************************************/
/* autoCorrelation2LP: convert autocorrelation coefficients to LP using      */
/*                     Levinson-Durbin algo described in spec 3.2.2          */
/*  parameters :                                                             */
/*    -(i) autoCorrelationCoefficients : 11 values in variable scale         */
/*         scale is not needed here as a division cancel it                  */
/*    -(o) LPCoefficientsQ12: 10 LP coefficients in Q12                      */
/*    -(o) reflectionCoefficient: 10 values Q31, k generated during Levinson */
/*         Durbin LP coefficient generation and needed for VAD and RFC3389   */
/*    -(o) residualEnergy : with the same scale factor as input              */
/*         autoCorrelationCoefficients, needed by DTX                        */
/*                                                                           */
/*****************************************************************************/
void autoCorrelation2LP(word32_t autoCorrelationCoefficients[], word16_t LPCoefficientsQ12[], word32_t reflectionCoefficients[], word32_t *residualEnergy) {
	/*********************************************************************************/
	/* Compute the LP Coefficient using Levinson-Durbin algo spec 3.2.2              */
	/*********************************************************************************/
	/* start a iteration i=2, init values as after iteration i=1 :                   */
	/*        a[0] = 1                                                               */
	/*        a[1] = -r1/r0                                                          */
	/*        E = r0(1 - a[1]^2)                                                     */
	/*                                                                               */
	/*  iterations i = 2..10                                                         */
	/*       sum = r[i] + ∑ a[j]*r[i-j] with j = 1..i-1 (a[0] is always 1)           */
	/*       a[i] = -sum/E                                                           */
	/*       iterations j = 1..i-1                                                   */
	/*            a[j] += a[i]*a{i-1}[i-j] use a{i-1}: from previous iteration       */
	/*       E *=(1-a[i]^2)                                                          */
	/*                                                                               */
	/*  r in Q31 (normalised) stored in array autoCorrelationCoefficients            */
	/*  E in Q31 (can't be > 1)                                                      */
	/*  sum in Q27 (sum can't be > 1 but intermediate accumulation can)              */
	/*  a in Q4.27 with full range possible                                          */
	/*      Note: during iteration, current a[i] is in Q31 (can't be >1) and is      */
	/*            set to Q27 at the end of current iteration                         */
	/*                                                                               */
	/*********************************************************************************/
	word32_t previousIterationLPCoefficients[NB_LSP_COEFF+1]; /* to compute a[]*/
	word32_t LPCoefficients[NB_LSP_COEFF+1]; /* in Q4.27 */

	word32_t E = 0; /* in Q31 */
	word32_t sum = 0; /* in Q27 */
	int i,j;

	/* init */
	LPCoefficients[0] = ONE_IN_Q27;
	LPCoefficients[1] = -DIV32_32_Q27(autoCorrelationCoefficients[1], autoCorrelationCoefficients[0]); /* result in Q27(but<1) */
	reflectionCoefficients[0] = SHL(LPCoefficients[1],4); /* k[0] is -r1/r0 in Q31 */
	/* E = r0(1 - a[1]^2) in Q31 */
	E = MULT32_32_Q31(autoCorrelationCoefficients[0], SUB32(ONE_IN_Q31, MULT32_32_Q23(LPCoefficients[1], LPCoefficients[1]))); /* LPCoefficient[1] is in Q27, using a Q23 operation will result in a Q31 variable */
	
	for (i=2; i<NB_LSP_COEFF+1; i++) {
		/* update the previousIterationLPCoefficients needed for this one */
		for (j=1; j<i; j++) {
			previousIterationLPCoefficients[j] = LPCoefficients[j];
		}

		/* sum = r[i] + ∑ a[j]*r[i-j] with j = 1..i-1 (a[0] is always 1)           */
		sum = 0;
		for (j=1; j<i; j++) {
			sum = MAC32_32_Q31(sum, LPCoefficients[j], autoCorrelationCoefficients[i-j]);/* LPCoefficients in Q27, autoCorrelation in Q31 -> result in Q27 -> sum in Q27 */
		}
		sum = ADD32(SHL(sum, 4), autoCorrelationCoefficients[i]); /* set sum in Q31 and add r[0] */
		
		/* a[i] = -sum/E                                                           */
		LPCoefficients[i] = -DIV32_32_Q31(sum,E); /* LPCoefficient of current iteration is in Q31 for now, it will be set to Q27 at the end of this iteration */
		reflectionCoefficients[i-1] = LPCoefficients[i]; /* k[1] is needed by VAD others by RFC3389 RTP payload for Comfort Noise spectral information encoding */

		/* iterations j = 1..i-1                                                   */
		/*      a[j] += a[i]*a[i-j]                                                */
		for (j=1; j<i; j++) {
			LPCoefficients[j] = MAC32_32_Q31(LPCoefficients[j], LPCoefficients[i], previousIterationLPCoefficients[i-j]); /*LPCoefficients in Q27 except for LPCoefficients[i] in Q31 */
		}
		/* E *=(1-a[i]^2)                                                          */
		E = MULT32_32_Q31(E, SUB32(ONE_IN_Q31, MULT32_32_Q31(LPCoefficients[i], LPCoefficients[i]))); /* all in Q31 */
		/* set LPCoefficients[i] from Q31 to Q27 */
		LPCoefficients[i] = SHR(LPCoefficients[i], 4);
	}
	*residualEnergy = E;

	/* convert with rounding the LP Coefficients form Q27 to Q12, ignore first coefficient which is always 1 */
	for (i=0; i<NB_LSP_COEFF; i++) {
		LPCoefficientsQ12[i] = (word16_t)SATURATE(PSHR(LPCoefficients[i+1], 15), MAXINT16);
	}
}


/*****************************************************************************/
/* computeLP : As described in spec 3.2.1 and 3.2.2 : Windowing,             */
/*      Autocorrelation and Levinson-Durbin algorithm                        */
/*    parameters:                                                            */
/*      -(i) signal: 240 samples in Q0, the last 40 are from next frame      */
/*      -(o) LPCoefficientsQ12: 10 LP coefficients in Q12                    */
/*      -(o) reflectionCoefficient: 10 values Q31, k generated by Levinson   */
/*         Durbin LP coefficient generation and needed for VAD and RFC3389   */
/*      -(o) reflectionCoefficient: in Q31, k[1] generated during Levinson   */
/*           Durbin LP coefficient generation and needed for VAD             */
/*      -(o) autoCorrelationCoefficients : used internally but needed by VAD */
/*            scale is variable                                              */
/*      -(o) noLagautoCorrelationCoefficients : needed by DTX                */
/*            scale is variable                                              */
/*      -(o) autoCorrelationCoefficientsScale : scale factor of previous buf */
/*      -(i) autoCorrelationCoefficientsNumber number of coeff to be computed*/
/*           13 if we are using them for VAD, only 11 otherwise              */
/*****************************************************************************/
void computeLP(word16_t signal[], word16_t LPCoefficientsQ12[], word32_t reflectionCoefficients[], word32_t autoCorrelationCoefficients[], word32_t noLagAutocorrelationCoefficients[], int8_t *autoCorrelationCoefficientsScale, uint8_t autoCorrelationCoefficientsNumber)
{
	int i,j;
	word16_t windowedSignal[L_LP_ANALYSIS_WINDOW];
	word64_t acc64=0; /* acc on 64 bits */ 
	int rightShiftToNormalise=0;
	word32_t residualEnergy; /* interally compute by autoCorrelation2LP and extracted for DTX only, useless here */

	/*********************************************************************/
	/* Compute the windowed signal according to spec 3.2.1 eq4           */
	/*********************************************************************/
	for (i=0; i<L_LP_ANALYSIS_WINDOW; i++) {
		windowedSignal[i] = MULT16_16_P15(signal[i], wlp[i]); /* signal in Q0, wlp in Q0.15, windowedSignal in Q0 */
	}

	/*********************************************************************************/
	/* Compute the autoCorrelation coefficients r[0..10] according to spec 3.2.1 eq5 */
	/*********************************************************************************/
	/* Compute autoCorrelationCoefficients[0] first as it is the highest number and normalise it on 32 bits then apply the same normalisation to the other coefficients */
	/* autoCorrelationCoefficients are normalised on 32 bits and then considered as Q31 in range [-1,1[ */
	/* autoCorrelationCoefficients[0] is computed on 64 bits as it is likely to overflow 32 bits */
	for (i=0; i<L_LP_ANALYSIS_WINDOW; i++) {
		acc64 = MAC64(acc64, windowedSignal[i], windowedSignal[i]);
	}
	if (acc64==0) {
		acc64 = 1; /* spec 3.2.1: To avoid arithmetic problems for low-level input signals the value of r(0) has a lower boundary of r(0) = 1.0 */
	}
	/* normalise the acc64 on 32 bits */
	if (acc64>MAXINT32) {
		do {
			acc64 = SHR(acc64,1);
			rightShiftToNormalise++;
		} while (acc64>MAXINT32);
		autoCorrelationCoefficients[0] = acc64;
	} else {
		rightShiftToNormalise = -countLeadingZeros((word32_t)acc64);
		autoCorrelationCoefficients[0] = SHL((word32_t)acc64, -rightShiftToNormalise);
	}

	/* give current autoCorrelation coefficients scale to the output */
	*autoCorrelationCoefficientsScale = -rightShiftToNormalise;

	/* compute autoCorrelationCoefficients 1 to requested number (10 - no VAD - or 12 if VAD is enabled */
	if (rightShiftToNormalise>0) { /* acc64 was not fitting on 32 bits so compute the other sum on 64 bits too */
		for (i=1; i<autoCorrelationCoefficientsNumber; i++) {
			/* compute the sum in the 64 bits acc*/
			acc64=0;
			for (j=i; j<L_LP_ANALYSIS_WINDOW; j++) {
				acc64 = ADD64_32(acc64, MULT16_16(windowedSignal[j], windowedSignal[j-i]));
			}
			/* normalise it */
			autoCorrelationCoefficients[i] = SHR(acc64 ,rightShiftToNormalise);
		}
	} else { /* acc64 was fitting on 32 bits, compute the other sum on 32 bits only as it is faster */
		for (i=1; i<autoCorrelationCoefficientsNumber; i++) {
			/* compute the sum in the 64 bits acc*/
			word32_t acc32=0;
			for (j=i; j<L_LP_ANALYSIS_WINDOW; j++) {
				acc32 = MAC16_16(acc32, windowedSignal[j], windowedSignal[j-i]);
			}
			/* normalise it */
			autoCorrelationCoefficients[i] = SHL(acc32, -rightShiftToNormalise);
		}
	}

	/* save autocorrelation before applying lag window as they are requested like this for DTX */
	for (i=0; i<autoCorrelationCoefficientsNumber; i++) {
		noLagAutocorrelationCoefficients[i] = autoCorrelationCoefficients[i];
	}

	/* apply lag window on the autocorrelation coefficients spec 3.2.1 eq7 */
	/* this check shall be useless but it makes some compiler happy */
	if (autoCorrelationCoefficientsNumber>NB_LSP_COEFF+3) {
		autoCorrelationCoefficientsNumber = NB_LSP_COEFF+3;
	}

	for (i=1; i<autoCorrelationCoefficientsNumber; i++) {
		autoCorrelationCoefficients[i] = MULT16_32_P15(wlag[i], autoCorrelationCoefficients[i]); /* wlag in Q15 */
		//autoCorrelationCoefficients[i] = MULT32_32_Q31(wlag[i], autoCorrelationCoefficients[i]); /* wlag in Q31 */
	}

	/* convert to LP using Levinson-Durbin algo described in sepc 3.2.2 */
	autoCorrelation2LP(autoCorrelationCoefficients, LPCoefficientsQ12, reflectionCoefficients, &residualEnergy);

	return;
}
