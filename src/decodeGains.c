/*
 decodeGains.c

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

#include "decodeGains.h"

/* init function */
void initDecodeGains(bcg729DecoderChannelContextStruct *decoderChannelContext)
{
	/*init previousGainPredictionError to -14 in Q10 */
	decoderChannelContext->previousGainPredictionError[0] = -14336;
	decoderChannelContext->previousGainPredictionError[1] = -14336;
	decoderChannelContext->previousGainPredictionError[2] = -14336;
	decoderChannelContext->previousGainPredictionError[3] = -14336;
}

/*****************************************************************************/
/* decodeGains : decode adaptive and fixed codebooks gains as in spec 4.1.5  */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i) GA : parameter GA: Gain Codebook Stage 1 (3 bits)               */
/*      -(i) GB : paremeter GB: Gain Codebook Stage 2 (4 bits)               */
/*      -(i) fixedCodebookVector: 40 values current fixed Codebook vector    */
/*           in Q1.13.                                                       */
/*      -(i) frameErasureFlag : set in case of frame erasure                 */
/*      -(i/o) adaptativeCodebookGain : input previous/output current        */
/*             subframe Pitch Gain in Q14                                    */
/*      -(i/o) fixedCodebookGain : input previous/output current Fixed       */
/*             Codebook Gain in Q1                                           */
/*                                                                           */
/*****************************************************************************/
void decodeGains (bcg729DecoderChannelContextStruct *decoderChannelContext, uint16_t GA, uint16_t GB, word16_t *fixedCodebookVector, uint8_t frameErasureFlag,
			word16_t *adaptativeCodebookGain, word16_t *fixedCodebookGain)
{
	word32_t predictedFixedCodebookGain;
	word16_t fixedCodebookGainCorrectionFactor;

	/* check the erasure flag */
	if (frameErasureFlag != 0) { /* we have a frame erasure, proceed as described in spec 4.4.2 */
		int i;
		word32_t currentGainPredictionError =0;

		/*  adaptativeCodebookGain as in eq94 */
		if (*adaptativeCodebookGain < 16384) { /* last subframe gain < 1 in Q14 */
			*adaptativeCodebookGain = MULT16_16_Q15(*adaptativeCodebookGain, 29491 );      /* *0.9 in Q15 */
		} else { /* bound current subframe gain to 0.9 (14746 in Q14) */
			*adaptativeCodebookGain = 14746;
		}
		/* fixedCodebookGain as in eq93 */
		*fixedCodebookGain = MULT16_16_Q15(*fixedCodebookGain, 32113 );      /* *0.98 in Q15 */

		/* And update the previousGainPredictionError according to spec 4.4.3 */
		for (i=0; i<4; i++) {
			currentGainPredictionError = ADD32(currentGainPredictionError, decoderChannelContext->previousGainPredictionError[i]); /* previousGainPredictionError in Q3.10-> Sum in Q5.10 (on 32 bits) */
		}
		currentGainPredictionError = PSHR(currentGainPredictionError,2); /* /4 -> Q3.10 */

		if (currentGainPredictionError < -10240) { /* final result is low bounded by -14, so check before doing -4 if it's over -10(-10240 in Q10) or not */
			currentGainPredictionError = -14336; /* set to -14 in Q10 */
		} else { /* substract 4 */
			currentGainPredictionError = SUB32(currentGainPredictionError,4096); /* in Q10 */
		}

		/* shift the array and insert the current Prediction Error */
		decoderChannelContext->previousGainPredictionError[3] = decoderChannelContext->previousGainPredictionError[2];
		decoderChannelContext->previousGainPredictionError[2] = decoderChannelContext->previousGainPredictionError[1];
		decoderChannelContext->previousGainPredictionError[1] = decoderChannelContext->previousGainPredictionError[0];
		decoderChannelContext->previousGainPredictionError[0] = (word16_t)currentGainPredictionError;

		return;
	}

	/* First recover the GA and GB real index from their mapping tables(spec 3.9.3) */
	GA = reverseIndexMappingGA[GA];
	GB = reverseIndexMappingGB[GB];

	/* Compute the adaptativeCodebookGain from the tables according to eq73 in spec3.9.2 */
	/* adaptativeCodebookGain = GACodebook[GA][0] + GBCodebook[GB][0] */
	*adaptativeCodebookGain = ADD16(GACodebook[GA][0], GBCodebook[GB][0]); /* result in Q1.14 */

	/* Fixed Codebook: MA code-gain prediction */
	predictedFixedCodebookGain = MACodeGainPrediction(decoderChannelContext->previousGainPredictionError, fixedCodebookVector); /* predictedFixedCodebookGain on 32 bits in Q11.16 */

	/* get fixed codebook gain correction factor(gama) from the codebooks GA and GB according to eq74 */
	fixedCodebookGainCorrectionFactor = ADD16(GACodebook[GA][1], GBCodebook[GB][1]); /* result in Q3.12 (range [0.185, 5.05])*/

	/* compute fixedCodebookGain according to eq74 */
	*fixedCodebookGain = (word16_t)PSHR(MULT16_32_Q12(fixedCodebookGainCorrectionFactor, predictedFixedCodebookGain), 15); /* Q11.16*Q3.12 -> Q14.16, shift by 15 to get a Q14.1 which fits on 16 bits */
	
	/* use eq72 to compute current prediction error in order to update the previousGainPredictionError array */
	computeGainPredictionError(fixedCodebookGainCorrectionFactor, decoderChannelContext->previousGainPredictionError);
}
