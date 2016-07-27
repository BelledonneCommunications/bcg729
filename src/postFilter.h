/*
 postFilter.h

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
#ifndef POSTFILTER_H
#define POSTFILTER_H
/* init function */
void initPostFilter(bcg729DecoderChannelContextStruct *decoderChannelContext);

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
		word16_t *postFilteredSignal);
#endif /* ifndef POSTFILTER_H */
