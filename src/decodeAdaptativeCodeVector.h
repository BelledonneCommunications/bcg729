/*
 decodeAdaptativeCodeVector.h

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
#ifndef DECODEADAPTATIVECODEVECTOR_H
#define DECODEADAPTATIVECODEVECTOR_H
/* init function */
void initDecodeAdaptativeCodeVector(bcg729DecoderChannelContextStruct *decoderChannelContext);

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
void computeAdaptativeCodebookVector(word16_t *excitationVector, int16_t fracPitchDelay, int16_t intPitchDelay);

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
				int16_t *intPitchDelay, word16_t *excitationVector);
#endif /* ifndef DECODEADAPTATIVECODEVECTOR_H */
