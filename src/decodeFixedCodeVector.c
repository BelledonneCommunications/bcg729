/*
 decodeFixedCodeVector.c

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

/*****************************************************************************/
/* decodeFixedCodeVector : compute the fixed codebook vector as in spec 4.1.4*/
/*    parameters:                                                            */
/*      -(i) signs: parameter S(4 signs bit) eq61                            */
/*      -(i) positions: parameter C(4 3bits position and jx bit) eq62        */
/*      -(i) intPitchDelay: integer part of pitch Delay (T)                  */
/*      -(i) boundedPitchGain: Beta in eq47 and eq48, in Q14                 */
/*      -(o) fixedCodebookVector: 40 values in Q13, range:  [-1.8,+1.8]      */
/*                                                                           */
/*****************************************************************************/
void decodeFixedCodeVector(uint16_t signs, uint16_t positions, int16_t intPitchDelay, word16_t boundedPitchGain,
				word16_t *fixedCodebookVector)
{
	uint16_t positionsArray[4];
	uint16_t jx;
	int i;

	/* get the positions into an array: mapping according to eq62 and table7 in spec 3.8 */
	positionsArray[0] = (positions&(uint16_t)7)*5; /* m0 = 5*C, do not use macro here as whatever fixed or floating point computation we use, these are integers */
	positions = SHR(positions, 3); /* shift right by 3 to get the 3 next bits(m1/5) as LSB */
	positionsArray[1] = ((positions&(uint16_t)7)*5) + 1; /* m1 = 5*C + 1, do not use macro here as whatever fixed or floating point computation we use, these are integers */
	positions = SHR(positions, 3); /* shift right by 3 to get the 3 next bits(m2/5) as LSB */
	positionsArray[2] = ((positions&(uint16_t)7)*5) + 2; /* m2 = 5*C + 2, do not use macro here as whatever fixed or floating point computation we use, these are integers */
	positions = SHR(positions, 3); /* shift right by 3 to get the last 4 bits(m3/5 and jx) as LSB */
	jx = positions&(uint16_t)1; /* jx from eq62 is the last bit */
	positions = SHR(positions, 1); /* shift right by 1 to get the last 3 bits as LSB */
	positionsArray[3] = ((positions&(uint16_t)7)*5) + 3 + jx; /* m3 = 5*C + 3 + jx, do not use macro here as whatever fixed or floating point computation we use, these are integers */
	
	/* initialise the output Vector */
	for (i=0; i<L_SUBFRAME; i++) {
		fixedCodebookVector[i]=0;
	}

	/* get the signs and compute the fixedCodebookVector */
	for (i=0; i<4; i++) {
		if ((signs&(uint16_t)1) != 0) { /* sign bit is 1 */
			fixedCodebookVector[positionsArray[i]] = 8192; /* +1 in Q13 */
		} else {
			fixedCodebookVector[positionsArray[i]] = -8192; /* -1 in Q13 */
		}
		signs = SHR(signs, 1); /* shift signs right by one to get the next sign bit at next loop */
	}

	/* if intPitchDelay is smaller than subframe length, give some correction using boundedPitchGain according to eq48 */
	for (i=intPitchDelay; i<L_SUBFRAME; i++) {
		/* fixedCodebookVector[i] = fixedCodebookVector[i] + fixedCodebookVector[i-intPitchDelay]*boundedPitchGain */
		fixedCodebookVector[i] = ADD16(fixedCodebookVector[i], MULT16_16_P14(fixedCodebookVector[i-intPitchDelay], boundedPitchGain));
	}
}
