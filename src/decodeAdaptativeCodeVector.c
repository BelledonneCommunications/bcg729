/*
 decodeAdaptativeCodeVector.c

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

#include "decodeAdaptativeCodeVector.h"

/* init function */
void initDecodeAdaptativeCodeVector(bcg729DecoderChannelContextStruct *decoderChannelContext)
{
	decoderChannelContext->previousIntPitchDelay = 60;
}


/*****************************************************************************/
/* computeAdaptativeCodeVector : as in spec 4.1.3                            */
/*    parameters:                                                            */
/*      -(i/o) excitationVector : in Q0 excitation accessed from             */
/*             [-MAXIMUM_INT_PITCH_DELAY(143), -1] as input                  */
/*             and [0, L_SUBFRAME[ as output to store the adaptative         */
/*             codebook vector                                               */
/*      -(i/o) fracPitchDelay : the fractionnal part of Pitch Delay.         */
/*      -(i/o) intPitchDelay : the integer part of Pitch Delay.              */
/*                                                                           */
/*****************************************************************************/
void computeAdaptativeCodebookVector(word16_t *excitationVector, int16_t fracPitchDelay, int16_t intPitchDelay) {
	word16_t *excitationVectorMinusK; /* pointer to u(-k) */
	int n;
	/* compute the adaptative codebook vector using the pitch delay and the past excitation vector */
	/* from spec 4.1.3 and 3.7.1 */
	/* shall compute v(n ) = ∑ u (n - k + i )b30 (t + 3i ) + ∑ u (n - k + 1 + i )b30 (3 - t + 3i ) for i=0,...,9 and n = 0,...,39 (t in 0, 1, 2) */
	/* with k = intPitchDelay and t = fracPitchDelay wich must be converted from range -1,0,1 to 0,1,2 */
	/* u the past excitation vector */
	/* v the adaptative codebook vector */
	/* b30 an interpolation filter */

	/* scale fracPichDelay from -1,0.1 to 0,1,2 */
	if (fracPitchDelay==1) {
		excitationVectorMinusK = &(excitationVector[-(intPitchDelay+1)]); /* fracPitchDelay being positive -> increase by one the integer part and set to 2 the fractional part : -(k+1/3) -> -(k+1)+2/3 */
		fracPitchDelay = 2;
	} else {
		fracPitchDelay = -fracPitchDelay; /* 0 unchanged, -1 -> +1 */
		excitationVectorMinusK = &(excitationVector[-intPitchDelay]); /* -(k-1/3) -> -k+1/3  or -(k) -> -k*/
	}

	for (n=0; n<L_SUBFRAME; n++) { /* loop over the whole subframe */
		word16_t *excitationVectorNMinusK = &(excitationVectorMinusK[n]); /* point to u(n-k), unscaled value, full range */
		word16_t *excitationVectorNMinusKPlusOne = &(excitationVectorMinusK[n+1]); /* point to u(n-k+1), unscaled value, full range */

		word16_t *b301 = &(b30[fracPitchDelay]); /* point to b30(t) in Q0.15 : sums of all b30 coeffs is < 2, no overflow possible on 32 bits */
		word16_t *b302 = &(b30[3-fracPitchDelay]); /* point to b30(3-t) in Q0.15*/
		int i,j; /* j will store 3i */
		word32_t acc = 0; /* in Q15 */
		for (i=0, j=0; i<10; i++, j+=3) {
			acc = MAC16_16(acc, excitationVectorNMinusK[-i], b301[j]); /*  Note : the spec says: u(n−k+i)b30(t+3i) but the ITU code do (and here too) u(n-k-i )b30(t+3i) */
			acc = MAC16_16(acc, excitationVectorNMinusKPlusOne[i], b302[j]); /* u(n-k+1+i)b30(3-t+3i) */
		}
		excitationVector[n] = SATURATE(PSHR(acc, 15), MAXINT16); /* acc in Q15, shift/round to unscaled value and check overflow on 16 bits */
	}

}

/*****************************************************************************/
/* decodeAdaptativeCodeVector : as in spec 4.1.3                             */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i) subFrameIndex : 0 or 40 for subframe 1 or subframe 2            */
/*      -(i) adaptativeCodebookIndex : parameter P1 or P2                    */
/*      -(i) parityFlag : based on P1 parity flag : set if parity error      */
/*      -(i) frameErasureFlag : set in case of frame erasure                 */
/*      -(i/o) intPitchDelay : the integer part of Pitch Delay. Computed from*/
/*             P1 on subframe 1. On Subframe 2, contains the intPitchDelay   */
/*             computed on Subframe 1.                                       */
/*      -(i/o) excitationVector : in Q0 excitation accessed from             */
/*             [-MAXIMUM_INT_PITCH_DELAY(143), -1] as input                  */
/*             and [0, L_SUBFRAME[ as output to store the adaptative         */
/*             codebook vector                                               */
/*                                                                           */
/*****************************************************************************/
void decodeAdaptativeCodeVector(bcg729DecoderChannelContextStruct *decoderChannelContext, int subFrameIndex, uint16_t adaptativeCodebookIndex, uint8_t parityFlag, uint8_t frameErasureFlag,
				int16_t *intPitchDelay, word16_t *excitationVector)
{
	int16_t fracPitchDelay;

	/*** Compute the Pitch Delay from the Codebook index ***/
	/* fracPitchDelay is computed in the range -1,0,1 */
	if (subFrameIndex == 0 ) { /* first subframe */
		if (parityFlag|frameErasureFlag) { /* there is an error (either parity or frame erased) */
			*intPitchDelay = decoderChannelContext->previousIntPitchDelay; /* set the integer part of Pitch Delay to the last second subframe Pitch Delay computed spec: 4.1.2 */
			/* Note: unable to find anything regarding this part in the spec, just copied it from the ITU source code */
			fracPitchDelay = 0;
			decoderChannelContext->previousIntPitchDelay++;
			if (decoderChannelContext->previousIntPitchDelay>MAXIMUM_INT_PITCH_DELAY) decoderChannelContext->previousIntPitchDelay=MAXIMUM_INT_PITCH_DELAY;
		} else { /* parity and frameErasure flags are off, do the normal computation (doc 4.1.3) */
			if (adaptativeCodebookIndex<197) {
				/* *intPitchDelay = (P1 + 2 )/ 3 + 19 */
				*intPitchDelay = ADD16(MULT16_16_Q15(ADD16(adaptativeCodebookIndex,2), 10923), 19); /* MULT in Q15: 1/3 in Q15: 10923 */
				/* fracPitchDelay = P1 − 3*intPitchDelay  + 58 : fracPitchDelay in -1, 0, 1 */
				fracPitchDelay = ADD16(SUB16(adaptativeCodebookIndex, MULT16_16(*intPitchDelay, 3)), 58);
			} else {/* adaptativeCodebookIndex>= 197 */
				*intPitchDelay = SUB16(adaptativeCodebookIndex, 112);
				fracPitchDelay = 0;
			}

			/* backup the intPitchDelay */
			decoderChannelContext->previousIntPitchDelay = *intPitchDelay;
		}
	} else { /* second subframe */
		if (frameErasureFlag) { /* there is an error : frame erased, in case of parity error, it has been taken in account at first subframe */
			/* unable to find anything regarding this part in the spec, just copied it from the ITU source code */
			*intPitchDelay = decoderChannelContext->previousIntPitchDelay;
			fracPitchDelay = 0;
			decoderChannelContext->previousIntPitchDelay++;
			if (decoderChannelContext->previousIntPitchDelay>MAXIMUM_INT_PITCH_DELAY) decoderChannelContext->previousIntPitchDelay=MAXIMUM_INT_PITCH_DELAY;
		} else { /* frameErasure flags are off, do the normal computation (doc 4.1.3) */
			int16_t tMin = SUB16(*intPitchDelay,5); /* intPitchDelay contains the intPitch computed for subframe one */
			if (tMin<20) {
				tMin = 20;
			}
			if (tMin>134) {
				tMin = 134;
			}
			/* intPitchDelay = (P2 + 2 )/ 3 − 1 */
			*intPitchDelay = SUB16(MULT16_16_Q15(ADD16(adaptativeCodebookIndex, 2), 10923), 1);
 			/* fracPitchDelay = P2 − 2 − 3((P 2 + 2 )/ 3 − 1) */
			fracPitchDelay = SUB16(SUB16(adaptativeCodebookIndex, MULT16_16(*intPitchDelay, 3)), 2);
			/* *intPitchDelay = (P2 + 2 )/ 3 − 1 + tMin */
			*intPitchDelay = ADD16(*intPitchDelay,tMin);

			/* backup the intPitchDelay */
			decoderChannelContext->previousIntPitchDelay = *intPitchDelay;
		}
	}

	/* compute the adaptative codebook vector using the pitch delay we just get and the past excitation vector */
	computeAdaptativeCodebookVector(excitationVector, fracPitchDelay, *intPitchDelay); 

	return;
}

