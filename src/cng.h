/*
 cng.h

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
#ifndef CNG_H
#define CNG_H


/*****************************************************************************/
/* initBcg729CNGChannel : create context structure and initialise it         */
/*    return value :                                                         */
/*      - the CNG channel context data                                       */
/*                                                                           */
/*****************************************************************************/
bcg729CNGChannelContextStruct *initBcg729CNGChannel();

void computeComfortNoiseExcitationVector(word16_t targetGain, uint16_t *randomGeneratorSeed, word16_t *excitationVector);
/*******************************************************************************************/
/* decodeSIDframe : as is spec B4.4 and B4.5                                               */
/*    first check if we have a SID frame or a missing/untransmitted frame                  */
/*    for SID frame get paremeters(gain and LSP)                                           */
/*    Then generate excitation vector and update qLSP                                      */
/*    parameters:                                                                          */
/*      -(i/o):CNGChannelContext : context containing all informations needed for CNG      */
/*      -(i): previousFrameIsActiveFlag: true if last decoded frame was an active one      */
/*      -(i): bitStream: for SID frame contains received params as in spec B4.3,           */
/*                       NULL for missing/untransmitted frame                              */
/*      -(i): bitStreamLength : in bytes, length of previous buffer                        */
/*      -(i/o): excitationVector in Q0, accessed in range [-L_PAST_EXCITATION,L_FRAME-1]   */
/*                                                        [-154,79]                        */
/*      -(i/o): previousqLSP : previous quantised LSP in Q0.15 (NB_LSP_COEFF values)       */
/*      -(o): LP: 20 LP coefficients in Q12                                                */
/*      -(i/o): pseudoRandomSeed : seed used in the pseudo random number generator         */
/*      -(i/o): previousLCodeWord: in Q2.13, buffer to store the last 4 frames codewords,  */
/*              used to compute the current qLSF                                           */
/*      -(i) rfc3389PayloadFlag: true when CN payload follow rfc3389                       */
/*                                                                                         */
/*******************************************************************************************/
void decodeSIDframe(bcg729CNGChannelContextStruct *CNGChannelContext, uint8_t previousFrameIsActiveFlag, uint8_t *bitStream, uint8_t bitStreamLength, word16_t *excitationVector, word16_t *previousqLSP, word16_t *LP, uint16_t *pseudoRandomSeed, word16_t previousLCodeWord[MA_MAX_K][NB_LSP_COEFF], uint8_t rfc3389PayloadFlag);
#endif /* ifndef CNG_H */
