/*
 dtx.h

 Copyright (C) 2015 Belledonne Communications, Grenoble, France
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
#ifndef DTX_H
#define DTX_H

/*****************************************************************************/
/* initBcg729DTXChannel : create context structure and initialise it         */
/*    return value :                                                         */
/*      - the DTX channel context data                                       */
/*                                                                           */
/*****************************************************************************/
bcg729DTXChannelContextStruct *initBcg729DTXChannel();

/*******************************************************************************************/
/* updateDTXContext : save autocorrelation value in DTX context as requested in B4.1.1     */
/*   parameters:                                                                           */
/*    -(i/o) DTXChannelContext : the DTX context to be updated                             */
/*    -(i) autocorrelationsCoefficients : 11 values of variable scale, values are copied   */
/*          in DTX context                                                                 */
/*    -(i) autocorrelationCoefficientsScale : the scale of previous buffer(can be <0)      */
/*                                                                                         */
/*******************************************************************************************/
void updateDTXContext(bcg729DTXChannelContextStruct *DTXChannelContext, word32_t *autocorrelationCoefficients, int8_t autocorrelationCoefficientsScale);

/*******************************************************************************************/
/* encodeSIDFrame: called at eache frame even if VADflag is set to active speech           */
/*   Update the previousVADflag and if curent is set to NOISE, compute the SID params      */
/*  parameters:                                                                            */
/*   -(i/o) DTXChannelContext: current DTX context, is updated by this function            */
/*   -(o)   previousLSPCoefficients : 10 values in Q15, is updated by this function        */
/*   -(i/o) previousqLSPCoefficients : 10 values in Q15, is updated by this function       */
/*   -(i) VADflag : 1 active voice frame, 0 noise frame                                    */
/*   -(i/o) previousqLSF : set of 4 last frames qLSF in Q2.13, is updated                  */
/*   -(i/o) excicationVector : in Q0, accessed in range [-L_PAST_EXCITATION,L_FRAME-1]     */
/*   -(o) qLPCoefficients : 20 values in Q3.12  the quantized LP coefficients              */
/*   -(o) bitStream : SID frame parameters on 2 bytes, may be null if no frame is to be    */
/*        transmitted                                                                      */
/*   -(o) bitStreamLength : length of bitStream buffer to be transmitted (2 for SID, 0 for */
/*        untransmitted frame)                                                             */
/*                                                                                         */
/*******************************************************************************************/
void encodeSIDFrame(bcg729DTXChannelContextStruct *DTXChannelContext,  word16_t *previousLSPCoefficients, word16_t *previousqLSPCoefficients,  uint8_t VADflag, word16_t previousqLSF[MA_MAX_K][NB_LSP_COEFF],  word16_t *excitationVector, word16_t *qLPCoefficients, uint8_t *bitStream, uint8_t *bitStreamLength);
#endif /* ifndef DTX_H */
