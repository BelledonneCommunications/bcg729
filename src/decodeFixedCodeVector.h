/*
 decodeFixedCodeVector.h

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
#ifndef DECODEFIXEDCODEVECTOR_H
#define DECODEFIXEDCODEVECTOR_H
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
				word16_t *fixedCodebookVector);
#endif /* ifndef DECODEFIXEDCODEVECTOR_H */
