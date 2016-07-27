/*
 preProcessing.c

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

/* coefficients a stored in Q1.12 for A1 (the only one having a float value > 1) and Q0.12 for all the others */
#define A1 ((word16_t)(7807))
#define A2 ((word16_t)(-3733))
#define B0 ((word16_t)(1899))
#define B1 ((word16_t)(-3798))
#define B2 ((word16_t)(1899))

/* Initialization of context values */
void initPreProcessing(bcg729EncoderChannelContextStruct *encoderChannelContext) {
	encoderChannelContext->outputY2 = 0;
	encoderChannelContext->outputY1 = 0;
	encoderChannelContext->inputX0   = 0;
	encoderChannelContext->inputX1   = 0;
}


/*****************************************************************************/
/* preProcessing : 2nd order highpass filter with cut off frequency at 140Hz */
/*      Algorithm:                                                           */
/*      y[i] = BO*x[i] + B1*x[i-1] + B2*x[i-2] + A1*y[i-1] + A2*y[i-2]       */
/*    parameters :                                                           */
/*      -(i/o) encoderChannelContext : the channel context data              */
/*      -(i) signal : 80 values in Q0                                        */
/*      -(o) preProcessedSignal : 80 values in Q0                            */
/*                                                                           */
/*****************************************************************************/
void preProcessing(bcg729EncoderChannelContextStruct *encoderChannelContext, word16_t signal[], word16_t preProcessedSignal[]) {
	int i;
	word16_t inputX2;
	word32_t acc; /* in Q12 */

	for(i=0; i<L_FRAME; i++) {
		inputX2 = encoderChannelContext->inputX1;
		encoderChannelContext->inputX1 = encoderChannelContext->inputX0;
		encoderChannelContext->inputX0 = signal[i];
	
		/* compute with acc and coefficients in Q12 */
		acc = MULT16_32_Q12(A1, encoderChannelContext->outputY1); /* Y1 in Q15.12 * A1 in Q1.12 -> acc in Q17.12*/
		acc = MAC16_32_Q12(acc, A2, encoderChannelContext->outputY2); /* Y2 in Q15.12 * A2 in Q0.12 -> Q15.12 + acc in Q17.12 -> acc in Q18.12 */
		/* 3*(Xi in Q15.0 * Bi in Q0.12)->Q17.12 + acc in Q18.12 -> acc in 19.12 */
		acc = MAC16_16(acc, encoderChannelContext->inputX0, B0);
		acc = MAC16_16(acc, encoderChannelContext->inputX1, B1);
		acc = MAC16_16(acc, inputX2, B2);
		/*  acc in Q19.12 : We must check it won't overflow 
			- the Q15.12 of Y
			- the Q15.0 extracted from it by shifting 12 right
		 -> saturate to 28 bits -> acc in Q15.12 */
		acc = SATURATE(acc, MAXINT28);
		
		preProcessedSignal[i] = PSHR(acc,12); /* extract integer value of the Q15.12 representation */
		encoderChannelContext->outputY2 = encoderChannelContext->outputY1;
		encoderChannelContext->outputY1 = acc;
	}
	return;
}
