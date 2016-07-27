/*
 postFilter.c

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

#include "typedef.h"
#include "codecParameters.h"
#include "basicOperationsMacros.h"
#include "utils.h"
#include "g729FixedPointMath.h"

/* init function */
void initPostFilter(bcg729DecoderChannelContextStruct *decoderChannelContext)
{
	/* set to zero the residual signal memory */
	memset(decoderChannelContext->residualSignalBuffer, 0, MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t));
	memset(decoderChannelContext->scaledResidualSignalBuffer, 0, MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t));
	/* set to zero the one word of longTermFilteredResidualSignal needed as memory for tilt compensation filter */
	decoderChannelContext->longTermFilteredResidualSignalBuffer[0] = 0;
	decoderChannelContext->longTermFilteredResidualSignal = &(decoderChannelContext->longTermFilteredResidualSignalBuffer[1]); /* init the pointer to the begining of longTermFilteredResidualSignal current subframe */
	/* intialise the shortTermFilteredResidualSignal filter memory and pointer*/
	memset(decoderChannelContext->shortTermFilteredResidualSignalBuffer, 0, NB_LSP_COEFF*sizeof(word16_t));
	decoderChannelContext->shortTermFilteredResidualSignal = &(decoderChannelContext->shortTermFilteredResidualSignalBuffer[NB_LSP_COEFF]);
	/* initialise the previous Gain for adaptative gain control */
	decoderChannelContext->previousAdaptativeGain = 4096; /* 1 in Q12 */
}

/*****************************************************************************/
/* postFilter: filter the reconstructed speech according to spec A.4.2       */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i) LPCoefficients: 10 LP coeff for current subframe in Q12         */
/*      -(i) reconstructedSpeech: output of LP Synthesis, 50 values in Q0    */
/*           10 values of previous subframe, accessed in range [-10, 39]     */
/*      -(i) intPitchDelay: the integer part of Pitch Delay in Q0            */
/*      -(i) subframeIndex: 0 or L_SUBFRAME for subframe 0 or 1              */
/*      -(o) postFilteredSignal: 40 values in Q0                             */
/*                                                                           */
/*****************************************************************************/
void postFilter(bcg729DecoderChannelContextStruct *decoderChannelContext, word16_t *LPCoefficients, word16_t *reconstructedSpeech, int16_t intPitchDelay, int subframeIndex,
		word16_t *postFilteredSignal)
{
	int i,j;
	word16_t LPGammaNCoefficients[NB_LSP_COEFF]; /* in Q12 */
	word16_t *residualSignal;
	word16_t *scaledResidualSignal;
	word32_t correlationMax = (word32_t)MININT32;
	int16_t intPitchDelayMax;
	int16_t bestIntPitchDelay = 0;
	word16_t *delayedResidualSignal;
	word32_t residualSignalEnergy = 0; /* in Q-4 */
	word32_t delayedResidualSignalEnergy = 0; /* in Q-4 */
	word32_t maximumThree;
	int16_t leadingZeros = 0;
	word16_t correlationMaxWord16 = 0;
	word16_t residualSignalEnergyWord16 = 0;
	word16_t delayedResidualSignalEnergyWord16 = 0;
	word16_t LPGammaDCoefficients[NB_LSP_COEFF]; /* in Q12 */
	word16_t hf[22]; /* the truncated impulse response to short term filter Hf in Q12 */
	word32_t rh1;
	word16_t tiltCompensatedSignal[L_SUBFRAME]; /* in Q0 */
	word16_t gainScalingFactor; /* in Q12 */
	uword32_t shortTermFilteredResidualSignalSquareSum = 0;

	/********************************************************************/
	/* Long Term Post Filter                                            */
	/********************************************************************/
	/*** Compute LPGammaN and LPGammaD coefficients : LPGamma[0] = LP[0]*Gamma^(i+1) (i=0..9) ***/
	/* GAMMA_XX constants are in Q15 */
	LPGammaNCoefficients[0] = MULT16_16_P15(LPCoefficients[0], GAMMA_N1);
	LPGammaNCoefficients[1] = MULT16_16_P15(LPCoefficients[1], GAMMA_N2);
	LPGammaNCoefficients[2] = MULT16_16_P15(LPCoefficients[2], GAMMA_N3);
	LPGammaNCoefficients[3] = MULT16_16_P15(LPCoefficients[3], GAMMA_N4);
	LPGammaNCoefficients[4] = MULT16_16_P15(LPCoefficients[4], GAMMA_N5);
	LPGammaNCoefficients[5] = MULT16_16_P15(LPCoefficients[5], GAMMA_N6);
	LPGammaNCoefficients[6] = MULT16_16_P15(LPCoefficients[6], GAMMA_N7);
	LPGammaNCoefficients[7] = MULT16_16_P15(LPCoefficients[7], GAMMA_N8);
	LPGammaNCoefficients[8] = MULT16_16_P15(LPCoefficients[8], GAMMA_N9);
	LPGammaNCoefficients[9] = MULT16_16_P15(LPCoefficients[9], GAMMA_N10);

	/*** Compute the residual signal as described in spec 4.2.1 eq79 ***/
	/* Compute also a scaled residual signal: shift right by 2 to avoid overflows on 32 bits when computing correlation and energy */
	
	/* pointers to current subframe beginning */
	residualSignal = &(decoderChannelContext->residualSignalBuffer[MAXIMUM_INT_PITCH_DELAY+subframeIndex]);
	scaledResidualSignal = &(decoderChannelContext->scaledResidualSignalBuffer[MAXIMUM_INT_PITCH_DELAY+subframeIndex]);

	for (i=0; i<L_SUBFRAME; i++) {
		word32_t acc = SHL((word32_t)reconstructedSpeech[i], 12); /* reconstructedSpeech in Q0 shifted to set acc in Q12 */
		for (j=0; j<NB_LSP_COEFF; j++) {
			acc = MAC16_16(acc, LPGammaNCoefficients[j],reconstructedSpeech[i-j-1]); /* LPGammaNCoefficients in Q12, reconstructedSpeech in Q0 -> acc in Q12 */
		}
		residualSignal[i] = (word16_t)SATURATE(PSHR(acc, 12), MAXINT16); /* shift back acc to Q0 and saturate it to avoid overflow when going back to 16 bits */
		scaledResidualSignal[i] = PSHR(residualSignal[i], 2); /* shift acc to Q-2 and saturate it to get the scaled version of the signal */
	}

	/*** Compute the maximum correlation on scaledResidualSignal delayed by intPitchDelay +/- 3 to get the best delay. Spec 4.2.1 eq80 ***/
	/* using a scaled(Q-2) signals gives correlation in Q-4. */
	intPitchDelayMax = intPitchDelay+3; /* intPitchDelayMax shall be < MAXIMUM_INT_PITCH_DELAY(143) */
	if (intPitchDelayMax>MAXIMUM_INT_PITCH_DELAY) {
		intPitchDelayMax = MAXIMUM_INT_PITCH_DELAY;
	}

	for (i=intPitchDelay-3; i<=intPitchDelayMax; i++) {
		word32_t correlation = 0;
		delayedResidualSignal = &(scaledResidualSignal[-i]); /* delayedResidualSignal points to scaledResidualSignal[-i] */
		
		/* compute correlation: ∑r(n)*rk(n) */
		for (j=0; j<L_SUBFRAME; j++) {
			correlation = MAC16_16(correlation, delayedResidualSignal[j], scaledResidualSignal[j]);
		}
		/* if we have a maximum correlation */
		if (correlation>correlationMax) {
			correlationMax = correlation;
			bestIntPitchDelay = i; /* get the intPitchDelay */
		}
	}

	/* saturate correlation to a positive integer */
	if (correlationMax<0) {
		correlationMax = 0;
	}

	/*** Compute the signal energy ∑r(n)*r(n) and delayed signal energy ∑rk(n)*rk(n) which shall be used to compute gl spec 4.2.1 eq81, eq 82 and eq83 ***/
	delayedResidualSignal = &(scaledResidualSignal[-bestIntPitchDelay]); /* in Q-2, points to the residual signal delayed to give the higher correlation: rk(n) */ 
	for (i=0; i<L_SUBFRAME; i++) {
		residualSignalEnergy = MAC16_16(residualSignalEnergy, scaledResidualSignal[i], scaledResidualSignal[i]);
		delayedResidualSignalEnergy = MAC16_16(delayedResidualSignalEnergy, delayedResidualSignal[i], delayedResidualSignal[i]);
	}

	/*** Scale correlationMax, residualSignalEnergy and delayedResidualSignalEnergy to the best fit on 16 bits ***/
	/* these variables must fit on 16bits for the following computation, to avoid loosing information, scale them */
	/* at best fit: scale the higher of three to get the value over 2^14 and shift the other two from the same amount */
	/* Note: all three value are >= 0 */
	maximumThree = correlationMax;
	if (maximumThree<residualSignalEnergy) {
		maximumThree = residualSignalEnergy;
	}
	if (maximumThree<delayedResidualSignalEnergy) {
		maximumThree = delayedResidualSignalEnergy;
	}

	if (maximumThree>0) { /* if all of them a null, just do nothing otherwise shift right to get the max number in range [0x4000,0x8000[ */
		leadingZeros = countLeadingZeros(maximumThree);
		if (leadingZeros<16) {
			correlationMaxWord16 = (word16_t)SHR32(correlationMax, 16-leadingZeros);
			residualSignalEnergyWord16 = (word16_t)SHR32(residualSignalEnergy, 16-leadingZeros);
			delayedResidualSignalEnergyWord16 = (word16_t)SHR32(delayedResidualSignalEnergy, 16-leadingZeros);
		} else { /* if the values already fit on 16 bits, no need to shift */
			correlationMaxWord16 = (word16_t)correlationMax;
			residualSignalEnergyWord16 = (word16_t)residualSignalEnergy;
			delayedResidualSignalEnergyWord16 = (word16_t)delayedResidualSignalEnergy;
		}
	}

	/* eq78: Hp(z)=(1 + γp*gl*z(−T))/(1 + γp*gl) -> (with g=γp*gl) Hp(z)=1/(1+g) + (g/(1+g))*z(-T) = g0 + g1*z(-T) */
	/* g = gl/2 (as γp=0.5)= (eq83) correlationMax/(2*delayedResidualSignalEnergy) */
	/* compute g0 = 1/(1+g)=  delayedResidualSignalEnergy/(delayedResidualSignalEnergy+correlationMax/2) = 1-g1*/
	/* compute g1 = g/(1+g) = correlationMax/(2*delayedResidualSignalEnergy+correlationMax) = 1-g0 */

	/*** eq82 -> (correlationMax^2)/(residualSignalEnergy*delayedResidualSignalEnergy)<0.5 ***/
	/* (correlationMax^2) < (residualSignalEnergy*delayedResidualSignalEnergy)*0.5 */
	if ((MULT16_16(correlationMaxWord16, correlationMaxWord16) < SHR(MULT16_16(residualSignalEnergyWord16, delayedResidualSignalEnergyWord16), 1)) /* eq82 */
		|| ((correlationMaxWord16==0) && (delayedResidualSignalEnergyWord16==0))) { /* correlationMax and delayedResidualSignalEnergy values are 0 -> unable to compute g0 and g1 -> disable filter */
		/* long term post filter disabled */
		for (i=0; i<L_SUBFRAME; i++) {
			decoderChannelContext->longTermFilteredResidualSignal[i] = residualSignal[i];
		}
	} else { /* eq82 gives long term filter enabled, */
		word16_t g0, g1;
		/* eq83: gl = correlationMax/delayedResidualSignalEnergy bounded in ]0,1] */
		/* check if gl > 1 -> gl=1 -> g=1/2 -> g0=2/3 and g1=1/3 */
		if (correlationMax > delayedResidualSignalEnergy) {
			g0 = 21845; /* 2/3 in Q15 */
			g1 = 10923; /* 1/3 in Q15 */
		} else {
			/* g1 = correlationMax/(2*delayedResidualSignalEnergy+correlationMax) */
			g1 = DIV32((word32_t)SHL32(correlationMaxWord16,15),(word32_t)ADD32(SHL32(delayedResidualSignalEnergyWord16,1), correlationMaxWord16)); /* g1 in Q15 */
			g0 = SUB16(32767, g1); /* g0 = 1 - g1 in Q15 */
		}
		
		/* longTermFilteredResidualSignal[i] = g0*residualSignal[i] + g1*delayedResidualSignal[i]*/
		delayedResidualSignal = &(residualSignal[-bestIntPitchDelay]);
		for (i=0; i<L_SUBFRAME; i++) {
			decoderChannelContext->longTermFilteredResidualSignal[i] = (word16_t)SATURATE(PSHR(ADD32(MULT16_16(g0, residualSignal[i]), MULT16_16(g1, delayedResidualSignal[i])), 15), MAXINT16);
		}
	}
	
	/********************************************************************/
	/* Tilt Compensation Filter                                         */
	/********************************************************************/

	/* compute hf the truncated (to 22 coefficients) impulse response of the filter A(z/γn)/A(z/γd) described in spec 4.2.2 eq84 */
	/* hf(i) = LPGammaNCoeff[i] - ∑[j:0..9]LPGammaDCoeff[j]*hf[i-j-1]) */
	/* GAMMA_XX constants are in Q15 */
	LPGammaDCoefficients[0] = MULT16_16_P15(LPCoefficients[0], GAMMA_D1);
	LPGammaDCoefficients[1] = MULT16_16_P15(LPCoefficients[1], GAMMA_D2);
	LPGammaDCoefficients[2] = MULT16_16_P15(LPCoefficients[2], GAMMA_D3);
	LPGammaDCoefficients[3] = MULT16_16_P15(LPCoefficients[3], GAMMA_D4);
	LPGammaDCoefficients[4] = MULT16_16_P15(LPCoefficients[4], GAMMA_D5);
	LPGammaDCoefficients[5] = MULT16_16_P15(LPCoefficients[5], GAMMA_D6);
	LPGammaDCoefficients[6] = MULT16_16_P15(LPCoefficients[6], GAMMA_D7);
	LPGammaDCoefficients[7] = MULT16_16_P15(LPCoefficients[7], GAMMA_D8);
	LPGammaDCoefficients[8] = MULT16_16_P15(LPCoefficients[8], GAMMA_D9);
	LPGammaDCoefficients[9] = MULT16_16_P15(LPCoefficients[9], GAMMA_D10);

	hf[0] = 4096; /* 1 in Q12 as LPGammaNCoefficients and LPGammaDCoefficient doesn't contain the first element which is 1 and past values of hf are 0 */
	for (i=1; i<11; i++) {
		word32_t acc = (word32_t)SHL(LPGammaNCoefficients[i-1],12); /* LPGammaNCoefficients in Q12 -> acc in Q24 */
		for (j=0; j<NB_LSP_COEFF && j<i; j++) { /* j<i to avoid access to negative index of hf(past values are 0 anyway) */
			acc = MSU16_16(acc, LPGammaDCoefficients[j], hf[i-j-1]); /* LPGammaDCoefficient in Q12, hf in Q12 -> Q24 TODO: Possible overflow?? */
		}
		hf[i] = (word16_t)SATURATE(PSHR(acc, 12), MAXINT16); /* get result back in Q12 and saturate on 16 bits */
	}
	for (i=11; i<22; i++) {
		word32_t acc = 0;
		for (j=0; j<NB_LSP_COEFF; j++) { /* j<i to avoid access to negative index of hf(past values are 0 anyway) */
			acc = MSU16_16(acc, LPGammaDCoefficients[j], hf[i-j-1]); /* LPGammaDCoefficient in Q12, hf in Q12 -> Q24 TODO: Possible overflow?? */
		}
		hf[i] = (word16_t)SATURATE(PSHR(acc, 12), MAXINT16); /* get result back in Q12 and saturate on 16 bits */
	}

	/* hf is then used to compute k'1 spec 4.2.3 eq87: k'1 = -rh1/rh0 */
	/* rh0 = ∑[i:0..21]hf[i]*hf[i] */
	/* rh1 = ∑[i:0..20]hf[i]*hf[i+1] */
	rh1 = MULT16_16(hf[0], hf[1]);
	for (i=1; i<21; i++) {
		rh1 = MAC16_16(rh1, hf[i], hf[i+1]); /* rh1 in Q24 */
	}

	/* tiltCompensationGain is set to 0 if k'1>0 -> rh1<0 (as rh0 is always>0) */
	if (rh1<0) { /* tiltCompensationGain = 0 -> no gain filter is off, just copy the input */
		memcpy(tiltCompensatedSignal, decoderChannelContext->longTermFilteredResidualSignal, L_SUBFRAME*sizeof(word16_t));
	} else { /*compute tiltCompensationGain = k'1*γt */
		word16_t tiltCompensationGain;
		word32_t rh0 = MULT16_16(hf[0], hf[0]);
		for (i=1; i<22; i++) {
			rh0 = MAC16_16(rh0, hf[i], hf[i]); /* rh0 in Q24 */
		}
		rh1 = MULT16_32_Q15(GAMMA_T, rh1); /* GAMMA_T in Q15, rh1 in Q24*/
		tiltCompensationGain = (word16_t)SATURATE((word32_t)(DIV32(rh1,PSHR(rh0,12))), MAXINT16); /* rh1 in Q24, PSHR(rh0,12) in Q12 -> tiltCompensationGain in Q12 */
		
		/* compute filter Ht (spec A.4.2.3 eqA14) = 1 + gain*z(-1) */
		for (i=0; i<L_SUBFRAME; i++) {
			tiltCompensatedSignal[i] = MSU16_16_Q12(decoderChannelContext->longTermFilteredResidualSignal[i], tiltCompensationGain, decoderChannelContext->longTermFilteredResidualSignal[i-1]);
		}
	}
	/* update memory word of longTermFilteredResidualSignal for next subframe */
	decoderChannelContext->longTermFilteredResidualSignal[-1] = decoderChannelContext->longTermFilteredResidualSignal[L_SUBFRAME-1];

	/********************************************************************/
	/* synthesis filter 1/[Â(z /γd)] spec A.4.2.2                       */
	/*                                                                  */
	/*   Note: Â(z/γn) was done before when computing residual signal   */
	/********************************************************************/
	/* shortTermFilteredResidualSignal is accessed in range [-NB_LSP_COEFF,L_SUBFRAME[ */
	synthesisFilter(tiltCompensatedSignal, LPGammaDCoefficients, decoderChannelContext->shortTermFilteredResidualSignal);
	/* get the last NB_LSP_COEFF of shortTermFilteredResidualSignal and set them as memory for next subframe(they do not overlap so use memcpy) */
	memcpy(decoderChannelContext->shortTermFilteredResidualSignalBuffer, &(decoderChannelContext->shortTermFilteredResidualSignalBuffer[L_SUBFRAME]), NB_LSP_COEFF*sizeof(word16_t));

	/********************************************************************/
	/* Adaptive Gain Control spec A.4.2.4                               */
	/*                                                                  */
	/********************************************************************/

	/*** compute G(gain scaling factor) according to eqA15 : G = Sqrt((∑s(n)^2)/∑sf(n)^2 ) ***/
	/* compute ∑sf(n)^2, scale the signal shifting right by 4 to avoid possible overflow on 32 bits sum */
	for (i=0; i<L_SUBFRAME; i++) {
		shortTermFilteredResidualSignalSquareSum = UMAC16_16_Q4(shortTermFilteredResidualSignalSquareSum, decoderChannelContext->shortTermFilteredResidualSignal[i], decoderChannelContext->shortTermFilteredResidualSignal[i]); /* inputs are both in Q0, output is in Q-4 */
	}
	
	/* if the sum is null we can't compute gain -> output of postfiltering is the output of shortTermFilter and previousAdaptativeGain is set to 0 */
	/* the reset of previousAdaptativeGain is not mentionned in the spec but in ITU code only */
	if (shortTermFilteredResidualSignalSquareSum == 0) {
		decoderChannelContext->previousAdaptativeGain = 0;
		for (i=0; i<L_SUBFRAME; i++) {
			postFilteredSignal[i] = decoderChannelContext->shortTermFilteredResidualSignal[i];
		}
	} else { /* we can compute adaptativeGain and output signal */
		word16_t currentAdaptativeGain;
		/* compute ∑s(n)^2 scale the signal shifting right by 4 to avoid possible overflow on 32 bits sum, same shift was applied at denominator */
		uword32_t reconstructedSpeechSquareSum = 0;
		for (i=0; i<L_SUBFRAME; i++) {
			reconstructedSpeechSquareSum = UMAC16_16_Q4(reconstructedSpeechSquareSum, reconstructedSpeech[i], reconstructedSpeech[i]); /* inputs are both in Q0, output is in Q-4 */
		}
		
		if (reconstructedSpeechSquareSum==0) { /* numerator is null -> current gain is null */
			gainScalingFactor = 0;	
		} else {
			uword32_t fractionResult; /* stores  ∑s(n)^2)/∑sf(n)^2 in Q10 on a 32 bit unsigned */
			word32_t scaledShortTermFilteredResidualSignalSquareSum;
			/* Compute ∑s(n)^2)/∑sf(n)^2  result shall be in Q10 */
			/* normalise the numerator on 32 bits */
			word16_t numeratorShift = unsignedCountLeadingZeros(reconstructedSpeechSquareSum);
			reconstructedSpeechSquareSum = USHL(reconstructedSpeechSquareSum, numeratorShift); /* reconstructedSpeechSquareSum*2^numeratorShift */

			/* normalise denominator to get the result directly in Q10 if possible */
			scaledShortTermFilteredResidualSignalSquareSum = VSHR32(shortTermFilteredResidualSignalSquareSum, 10-numeratorShift); /* shortTermFilteredResidualSignalSquareSum*2^(numeratorShift-10)*/
			
			if (scaledShortTermFilteredResidualSignalSquareSum==0) {/* shift might have sent to zero the denominator */
				fractionResult = UDIV32(reconstructedSpeechSquareSum, shortTermFilteredResidualSignalSquareSum); /* result in QnumeratorShift */
				fractionResult = VSHR32(fractionResult, numeratorShift-10); /* result in Q10 */
			} else { /* ok denominator is still > 0 */
				fractionResult = UDIV32(reconstructedSpeechSquareSum, scaledShortTermFilteredResidualSignalSquareSum); /* result in Q10 */
			}
			/* now compute current Gain =  Sqrt((∑s(n)^2)/∑sf(n)^2 ) */
			/* g729Sqrt_Q0Q7(Q0)->Q7, by giving a Q10 as input, output is in Q12 */
			gainScalingFactor = (word16_t)SATURATE(g729Sqrt_Q0Q7(fractionResult), MAXINT16);

			/* multiply by 0.1 as described in spec A.4.2.4 */
			gainScalingFactor = MULT16_16_P15(gainScalingFactor, 3277); /* in Q12, 3277 = 0.1 in Q15*/
		}
		/* Compute the signal according to eq89 (spec 4.2.4 and section A4.2.4) */
		/* currentGain = 0.9*previousGain + 0.1*gainScalingFactor the 0.1 factor has already been integrated in the variable gainScalingFactor */
		/* outputsignal = currentGain*shortTermFilteredResidualSignal */
		currentAdaptativeGain = decoderChannelContext->previousAdaptativeGain;
		for (i=0; i<L_SUBFRAME; i++) {
			currentAdaptativeGain = ADD16(gainScalingFactor, MULT16_16_P15(currentAdaptativeGain, 29491)); /* 29492 = 0.9 in Q15, result in Q12 */
			postFilteredSignal[i] = MULT16_16_Q12(currentAdaptativeGain, decoderChannelContext->shortTermFilteredResidualSignal[i]);
		}
		decoderChannelContext->previousAdaptativeGain = currentAdaptativeGain;
	}

	/* shift buffers if needed */
	if (subframeIndex>0) { /* only after 2nd subframe treatment */
		/* shift left by L_FRAME the residualSignal and scaledResidualSignal buffers */
		memmove(decoderChannelContext->residualSignalBuffer, &(decoderChannelContext->residualSignalBuffer[L_FRAME]), MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t));
		memmove(decoderChannelContext->scaledResidualSignalBuffer, &(decoderChannelContext->scaledResidualSignalBuffer[L_FRAME]), MAXIMUM_INT_PITCH_DELAY*sizeof(word16_t));
	}
	return;
}
