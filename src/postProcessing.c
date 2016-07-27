/*
 postProcessing.c

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

#include "preProcessing.h"

/*****************************************************************************/
/*                                                                           */
/* Define filter coefficients                                                */
/* Coefficient are given by the filter transfert function :                  */
/*                                                                           */
/*          0.46363718 - 0.92724705z(-1) + 0.46363718z(-2)                   */
/* H(z) = −−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−                  */
/*           1 - 1.9059465z(-1) + 0.9114024z(-2)                             */
/*                                                                           */
/* giving:                                                                   */
/*    y[i] = B0*x[i] + B1*x[i-1] + B2*x[i-2]                                 */
/*                   + A1*y[i-1] + A2*y[i-2]                                 */
/*                                                                           */
/*****************************************************************************/

/* coefficients are stored in Q1.13 */
#define A1 ((word16_t)(15836))
#define A2 ((word16_t)(-7667))
#define B0 ((word16_t)(7699))
#define B1 ((word16_t)(-15398))
#define B2 ((word16_t)(7699))

/* Initialization of context values */
void initPostProcessing(bcg729DecoderChannelContextStruct *decoderChannelContext) {
	decoderChannelContext->outputY2 = 0;
	decoderChannelContext->outputY1 = 0;
	decoderChannelContext->inputX0   = 0;
	decoderChannelContext->inputX1   = 0;
}


/*****************************************************************************/
/* postProcessing : high pass filtering and upscaling Spec 4.2.5             */
/*      Algorithm:                                                           */
/*      y[i] = BO*x[i] + B1*x[i-1] + B2*x[i-2] + A1*y[i-1] + A2*y[i-2]       */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i/o) signal : 40 values in Q0, reconstructed speech, output        */
/*             replaces the input in buffer                                  */
/*                                                                           */
/*****************************************************************************/
void postProcessing(bcg729DecoderChannelContextStruct *decoderChannelContext, word16_t signal[]) {
	int i;
	word16_t inputX2;
	word32_t acc; /* in Q13 */

	for(i=0; i<L_SUBFRAME; i++) {
		inputX2 = decoderChannelContext->inputX1;
		decoderChannelContext->inputX1 = decoderChannelContext->inputX0;
		decoderChannelContext->inputX0 = signal[i];
	
		/* compute with acc and coefficients in Q13 */
		acc = MULT16_32_Q13(A1, decoderChannelContext->outputY1); /* Y1 in Q14.13 * A1 in Q1.13 -> acc in Q17.13*/
		acc = MAC16_32_Q13(acc, A2, decoderChannelContext->outputY2); /* Y2 in Q14.13 * A2 in Q0.13 -> Q15.13 + acc in Q17.13 -> acc in Q18.12 */
		/* 3*(Xi in Q15.0 * Bi in Q0.13)->Q17.13 + acc in Q18.13 -> acc in 19.13(Overflow??) */
		acc = MAC16_16(acc, decoderChannelContext->inputX0, B0);
		acc = MAC16_16(acc, decoderChannelContext->inputX1, B1);
		acc = SATURATE(MAC16_16(acc, inputX2, B2), MAXINT29); /* saturate the acc to keep in Q15.13 */
		
		signal[i] = SATURATE(PSHR(acc,12), MAXINT16); /* acc in Q13 -> *2 and scale back to Q0 */
		decoderChannelContext->outputY2 = decoderChannelContext->outputY1;
		decoderChannelContext->outputY1 = acc;
	}
	return;
}
