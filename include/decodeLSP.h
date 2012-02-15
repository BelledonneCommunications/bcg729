/*
 decodeLSP.h

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef DECODELSP_H
#define DECODELSP_H

/*****************************************************************************/
/* initDecodeLSP :                                                           */
/*      Initialise the decodeLSP previousqLSP buffer                         */
/*                                                                           */
/*****************************************************************************/
void initDecodeLSP(bcg729DecoderChannelContextStruct *decoderChannelContext);

/*****************************************************************************/
/* decodeLSP : decode LSP coefficients as in spec 4.1.1/3.2.4                */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i) L: 4 elements array containing L[0-3] the first and             */
/*                     second stage vector of LSP quantizer                  */
/*      -(i) frameErased : a boolean, when true, frame has been erased       */
/*      -(o) qLSP: 10 quantized LSP coefficients in Q15 in range [-1,+1[     */
/*                                                                           */
/*****************************************************************************/
void decodeLSP(bcg729DecoderChannelContextStruct *decoderChannelContext, uint16_t L[], word16_t qLSP[], uint8_t frameErased);

#endif /* ifndef DECODELSP_H */
