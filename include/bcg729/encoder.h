/*
 encoder.h

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
#ifndef ENCODER_H
#define ENCODER_H
#include <stdint.h>
typedef struct bcg729EncoderChannelContextStruct_struct bcg729EncoderChannelContextStruct;

#ifdef WIN32
#define BCG729_VISIBILITY
#else
#define BCG729_VISIBILITY __attribute__ ((visibility ("default")))
#endif

/*****************************************************************************/
/* initBcg729EncoderChannel : create context structure and initialise it     */
/*    return value :                                                         */
/*      - the encoder channel context data                                   */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY bcg729EncoderChannelContextStruct *initBcg729EncoderChannel();

/*****************************************************************************/
/* closeBcg729EncoderChannel : free memory of context structure              */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : the channel context data                */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY void closeBcg729EncoderChannel(bcg729EncoderChannelContextStruct *encoderChannelContext);

/*****************************************************************************/
/* bcg729Encoder :                                                           */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : context for this encoder channel        */
/*      -(i) inputFrame : 80 samples (16 bits PCM)                           */
/*      -(o) bitStream : The 15 parameters for a frame on 80 bits            */
/*           on 80 bits (5 16bits words)                                     */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY void bcg729Encoder(bcg729EncoderChannelContextStruct *encoderChannelContext, int16_t inputFrame[], uint8_t bitStream[]);
#endif /* ifndef ENCODER_H */
