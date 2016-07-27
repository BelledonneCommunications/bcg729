/*
 decoder.h

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
#ifndef DECODER_H
#define DECODER_H
typedef struct bcg729DecoderChannelContextStruct_struct bcg729DecoderChannelContextStruct;
#include <stdint.h>

#ifdef _WIN32
#define BCG729_VISIBILITY
#else
#define BCG729_VISIBILITY __attribute__ ((visibility ("default")))
#endif

/*****************************************************************************/
/* initBcg729DecoderChannel : create context structure and initialise it     */
/*    return value :                                                         */
/*      - the decoder channel context data                                   */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY bcg729DecoderChannelContextStruct *initBcg729DecoderChannel();

/*****************************************************************************/
/* closeBcg729DecoderChannel : free memory of context structure              */
/*    parameters:                                                            */
/*      -(i) decoderChannelContext : the channel context data                */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY void closeBcg729DecoderChannel(bcg729DecoderChannelContextStruct *decoderChannelContext);

/*****************************************************************************/
/* bcg729Decoder :                                                           */
/*    parameters:                                                            */
/*      -(i) decoderChannelContext : the channel context data                */
/*      -(i) bitStream : 15 parameters on 80 bits                            */
/*      -(i): bitStreamLength : in bytes, length of previous buffer          */
/*      -(i) frameErased: flag: true, frame has been erased                  */
/*      -(i) SIDFrameFlag: flag: true, frame is a SID one                    */
/*      -(i) rfc3389PayloadFlag: true when CN payload follow rfc3389         */
/*      -(o) signal : a decoded frame 80 samples (16 bits PCM)               */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY void bcg729Decoder(bcg729DecoderChannelContextStruct *decoderChannelContext, uint8_t bitStream[], uint8_t bitStreamLength, uint8_t frameErasureFlag, uint8_t SIDFrameFlag, uint8_t rfc3389PayloadFlag, int16_t signal[]);
#endif /* ifndef DECODER_H */
