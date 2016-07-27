/*
 gainQuantization.c

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
#include "utils.h"
#include "codebooks.h"

#include "gainQuantization.h"

void initGainQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext)
{
	/*init previousGainPredictionError to -14 in Q10 */
	encoderChannelContext->previousGainPredictionError[0] = -14336;
	encoderChannelContext->previousGainPredictionError[1] = -14336;
	encoderChannelContext->previousGainPredictionError[2] = -14336;
	encoderChannelContext->previousGainPredictionError[3] = -14336;
}

/*****************************************************************************/
/* gainQuantization : compute quantized adaptative and fixed codebooks gains */
/*      spec 3.9                                                             */
/*    parameters:                                                            */
/*      -(i/o) encoderChannelContext : the channel context data              */
/*      -(i) targetSignal: 40 values in Q0, x in eq63                        */
/*      -(i) filteredAdaptativeCodebookVector: 40 values in Q0, y in eq63    */
/*      -(i) convolvedFixedCodebookVector: 40 values in Q12, z in eq63       */
/*      -(i) fixedCodebookVector: 40 values in Q13                           */
/*      -(i) xy in Q0 on 64 bits term of eq63 computed previously            */
/*      -(i) yy in Q0 on 64 bits term of eq63 computed previously            */
/*      -(o) quantizedAdaptativeCodebookGain : in Q14                        */
/*      -(o) quantizedFixedCodebookGain : in Q1                              */
/*      -(o) gainCodebookStage1 : GA parameter value (3 bits)                */
/*      -(o) gainCodebookStage2 : GB parameter value (4 bits)                */
/*                                                                           */
/*****************************************************************************/
void gainQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext, word16_t targetSignal[], word16_t filteredAdaptativeCodebookVector[], word16_t convolvedFixedCodebookVector[], word16_t fixedCodebookVector[], word64_t xy64, word64_t yy64,
					word16_t *quantizedAdaptativeCodebookGain, word16_t *quantizedFixedCodebookGain, uint16_t *gainCodebookStage1, uint16_t *gainCodebookStage2)
{
	int i,j;
	word64_t xz64=0, yz64=0, zz64=0;
	word32_t xy;
	word32_t yy;
	word32_t xz;
	word32_t yz;
	word32_t zz;
	uint16_t minNormalization = 31;
	uint16_t currentNormalization;
	word32_t bestAdaptativeCodebookGain, bestFixedCodebookGain;
	word64_t denominator;
	word16_t predictedFixedCodebookGain;
	uint16_t indexBaseGa=0;
	uint16_t indexBaseGb=0;
	uint16_t indexGa=0, indexGb=0;
	word64_t distanceMin = MAXINT64;

	/*** compute spec 3.9 eq63 terms first on 64 bits and then scale them if needed to fit on 32 ***/
	/* Xy64 and Yy64 already computed during adaptativeCodebookGain computation */
	for (i=0; i<L_SUBFRAME; i++) {
		xz64 = MAC64(xz64, targetSignal[i], convolvedFixedCodebookVector[i]); /* in Q12 */
		yz64 = MAC64(yz64, filteredAdaptativeCodebookVector[i], convolvedFixedCodebookVector[i]); /* in Q12 */
		zz64 = MAC64(zz64, convolvedFixedCodebookVector[i], convolvedFixedCodebookVector[i]); /* in Q24 */
	}
	
	/* now scale this terms to have them fit on 32 bits - terms Xy, Xz and Yz shall fit on 31 bits because used in eq63 with a factor 2 */
	xy = SHR64(((xy64<0)?-xy64:xy64),30);
	yy = SHR64(yy64,31);
	xz = SHR64(((xz64<0)?-xz64:xz64),30);
	yz = SHR64(((yz64<0)?-yz64:yz64),30);
	zz = SHR64(zz64,31);
	
	currentNormalization = countLeadingZeros(xy);
	if (currentNormalization<minNormalization) {
		minNormalization = currentNormalization;
	}
	currentNormalization = countLeadingZeros(xz);
	if (currentNormalization<minNormalization) {
		minNormalization = currentNormalization;
	}
	currentNormalization = countLeadingZeros(yz);
	if (currentNormalization<minNormalization) {
		minNormalization = currentNormalization;
	}
	currentNormalization = countLeadingZeros(yy);
	if (currentNormalization<minNormalization) {
		minNormalization = currentNormalization;
	}
	currentNormalization = countLeadingZeros(zz);
	if (currentNormalization<minNormalization) {
		minNormalization = currentNormalization;
	}

	if (minNormalization<31) { /* we shall normalise, values are over 32 bits */
		minNormalization = 31 - minNormalization;
		xy = (word32_t)SHR64(xy64, minNormalization);
		yy = (word32_t)SHR64(yy64, minNormalization);
		xz = (word32_t)SHR64(xz64, minNormalization);
		yz = (word32_t)SHR64(yz64, minNormalization);
		zz = (word32_t)SHR64(zz64, minNormalization);
		
	} else { /* no need to normalise, values already fit on 32 bits, just cast them */
		xy = (word32_t)xy64; /* in Q0 */
		yy = (word32_t)yy64; /* in Q0 */
		xz = (word32_t)xz64; /* in Q12 */
		yz = (word32_t)yz64; /* in Q12 */
		zz = (word32_t)zz64; /* in Q24 */
	}
	
	/*** compute the best gains minimizinq eq63 ***/
	/* Note this bestgain computation is not at all described in the spec, got it from ITU code */
	/* bestAdaptativeCodebookGain = (zz.xy - xz.yz) / (yy*zz) - yz^2) */
	/* bestfixedCodebookGain = (yy*xz - xy*yz) / (yy*zz) - yz^2) */
	/* best gain are computed in Q9 and Q2 and fits on 16 bits */
	denominator = MAC64(MULT32_32(yy, zz), -yz, yz); /* (yy*zz) - yz^2) in Q24 (always >= 0)*/
	/* avoid division by zero */
	if (denominator==0) { /* consider it to be one */
		bestAdaptativeCodebookGain = (word32_t)(SHR64(MAC64(MULT32_32(zz, xy), -xz, yz), 15)); /* MAC in Q24 -> Q9 */
		bestFixedCodebookGain = (word32_t)(SHR64(MAC64(MULT32_32(yy, xz), -xy, yz), 10)); /* MAC in Q12 -> Q2 */
	} else {
		/* bestAdaptativeCodebookGain in Q9 */ 
		uint16_t numeratorNorm;
		word64_t numerator = MAC64(MULT32_32(zz, xy), -xz, yz); /* in Q24 */
		/* check if we can shift it by 9 without overflow as the bestAdaptativeCodebookGain in computed in Q9 */
		word32_t numeratorH = (word32_t)(SHR64(numerator,32));
		numeratorH = (numeratorH>0)?numeratorH:-numeratorH;
		numeratorNorm = countLeadingZeros(numeratorH);
		if (numeratorNorm >= 9) {
			bestAdaptativeCodebookGain = (word32_t)(DIV64(SHL64(numerator,9), denominator)); /* bestAdaptativeCodebookGain in Q9 */
		} else {
			word64_t shiftedDenominator = SHR64(denominator, 9-numeratorNorm);
			if (shiftedDenominator>0) { /* can't shift left by 9 the numerator, can we shift right by 9-numeratorNorm the denominator without hiting 0 */
				bestAdaptativeCodebookGain = (word32_t)(DIV64(SHL64(numerator, numeratorNorm),shiftedDenominator)); /* bestAdaptativeCodebookGain in Q9 */
			} else {
				bestAdaptativeCodebookGain = SHL((word32_t)(DIV64(SHL64(numerator, numeratorNorm), denominator)), 9-numeratorNorm); /* shift left the division result to reach Q9 */
			}
		}

		numerator = MAC64(MULT32_32(yy, xz), -xy, yz); /* in Q12 */
		/* check if we can shift it by 14(it's in Q12 and denominator in Q24) without overflow as the bestFixedCodebookGain in computed in Q2 */
		numeratorH = (word32_t)(SHR64(numerator,32));
		numeratorH = (numeratorH>0)?numeratorH:-numeratorH;
		numeratorNorm = countLeadingZeros(numeratorH);

		if (numeratorNorm >= 14) {
			bestFixedCodebookGain = (word32_t)(DIV64(SHL64(numerator,14), denominator)); 
		} else {
			word64_t shiftedDenominator = SHR64(denominator, 14-numeratorNorm); /* bestFixedCodebookGain in Q14 */
			if (shiftedDenominator>0) { /* can't shift left by 9 the numerator, can we shift right by 9-numeratorNorm the denominator without hiting 0 */
				bestFixedCodebookGain = (word32_t)(DIV64(SHL64(numerator, numeratorNorm),shiftedDenominator)); /* bestFixedCodebookGain in Q14 */
			} else {
				bestFixedCodebookGain = SHL((word32_t)(DIV64(SHL64(numerator, numeratorNorm), denominator)), 14-numeratorNorm); /* shift left the division result to reach Q14 */
			}
		}
	}

	/*** Compute the predicted gain as in spec 3.9.1 eq71 in Q6 ***/
	predictedFixedCodebookGain = (word16_t)(SHR32(MACodeGainPrediction(encoderChannelContext->previousGainPredictionError, fixedCodebookVector), 12)); /* in Q16 -> Q4 range [3,1830] */

	/***  preselection spec 3.9.2 ***/
	/* Note: spec just says to select the best 50% of each vector, ITU code go through magical constant computation to select the begining of a continuous range */
	/* much more simple here : vector are ordened in growing order so just select 2 (4 for Gb) indexes before the first value to be superior to the best gain previously computed */
	while (indexBaseGa<6 && bestFixedCodebookGain>(MULT16_16_Q14(GACodebook[indexBaseGa][1],predictedFixedCodebookGain))) { /* bestFixedCodebookGain> in Q2, GACodebook in Q12 *predictedFixedCodebookGain in Q4 -> Q16-14 */
		indexBaseGa++;
	}
	if (indexBaseGa>0) indexBaseGa--;
	if (indexBaseGa>0) indexBaseGa--;
	while (indexBaseGb<12 && bestAdaptativeCodebookGain>(SHR(GBCodebook[indexBaseGb][0],5))) {
		indexBaseGb++;
	}
	if (indexBaseGb>0) indexBaseGb--;
	if (indexBaseGb>0) indexBaseGb--;
	if (indexBaseGb>0) indexBaseGb--;
	if (indexBaseGb>0) indexBaseGb--;

	/*** test all possibilities of Ga and Gb indexes and select the best one ***/
	xy = -SHL(xy,1); /* xy term is always used with a -2 factor */
	xz = -SHL(xz,1); /* xz term is always used with a -2 factor */
	yz = SHL(yz,1); /* yz term is always used with a 2 factor */

	for (i=0; i<4; i++) {
		for (j=0; j<8; j++) {
			/* compute gamma->gc and gp */
			word16_t gp =  ADD16(GACodebook[i+indexBaseGa][0], GBCodebook[j+indexBaseGb][0]); /* result in Q14 */
			word16_t gamma =  ADD16(GACodebook[i+indexBaseGa][1], GBCodebook[j+indexBaseGb][1]); /* result in Q3.12 (range [0.185, 5.05])*/
			word32_t gc = MULT16_16_Q14(gamma, predictedFixedCodebookGain); /* gamma in Q12, predictedFixedCodebookGain in Q4 -> Q16 -14 -> Q2 */
			
			/* compute E as in eq63 (first term excluded) */
			word64_t acc = MULT32_32(MULT16_16(gp, gp), yy); /* acc = gp^2*yy  gp in Q14, yy in Q0 -> acc in Q28 */
			acc = MAC64(acc, MULT16_16(gc, gc), zz); /* gc in Q2, zz in Q24 -> acc in Q28, note gc is on 32 bits but in a range making gc^2 fitting on 32 bits */
			acc = MAC64(acc, SHL32((word32_t)gp, 14), xy); /* gp in Q14 shifted to Q28, xy in Q0 -> acc in Q28 */
			acc = MAC64(acc, SHL32(gc, 14), xz); /* gc in Q2 shifted to Q16, xz in Q12 -> acc in Q28 */
			acc = MAC64(acc, MULT16_16(gp,gc), yz); /* gp in Q14, gc in Q2 yz in Q12 -> acc in Q28 */
			
			if (acc<distanceMin) {
				distanceMin = acc;
				indexGa = i+indexBaseGa;
				indexGb = j+indexBaseGb;
				*quantizedAdaptativeCodebookGain = gp;
				*quantizedFixedCodebookGain = (word16_t)SHR(gc, 1);
			}
		}
	}

	/* update the previous gain prediction error */
	computeGainPredictionError(ADD16(GACodebook[indexGa][1], GBCodebook[indexGb][1]), encoderChannelContext->previousGainPredictionError);

	/* mapping of indexes */
	*gainCodebookStage1 = indexMappingGA[indexGa];
	*gainCodebookStage2 = indexMappingGB[indexGb];

	return;
}
