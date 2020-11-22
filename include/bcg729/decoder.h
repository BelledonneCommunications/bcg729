/*
 * Copyright (c) 2011-2019 Belledonne Communications SARL.
 *
 * This file is part of bcg729.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DECODER_H
#define DECODER_H
typedef struct bcg729DecoderChannelContextStruct_struct bcg729DecoderChannelContextStruct;
#include <stdint.h>

// Version number is 1.1.1, map it on an integer
// Note: This define starts with version 1.1.1
#define BCG729_VERSION_NUMBER 0x010101

#ifdef _WIN32
	#ifdef BCG729_STATIC
		#define BCG729_VISIBILITY
	#else
		#ifdef BCG729_EXPORTS
			#define BCG729_VISIBILITY __declspec(dllexport)
		#else
			#define BCG729_VISIBILITY __declspec(dllimport)
		#endif
	#endif
#else
	#define BCG729_VISIBILITY __attribute__ ((visibility ("default")))
#endif

/*****************************************************************************/
/* initBcg729DecoderChannel : create context structure and initialise it     */
/*    return value :                                                         */
/*      - the decoder channel context data                                   */
/*                                                                           */
/*****************************************************************************/
BCG729_VISIBILITY bcg729DecoderChannelContextStruct *initBcg729DecoderChannel(void);

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
BCG729_VISIBILITY void bcg729Decoder(bcg729DecoderChannelContextStruct *decoderChannelContext, const uint8_t bitStream[], uint8_t bitStreamLength, uint8_t frameErasureFlag, uint8_t SIDFrameFlag, uint8_t rfc3389PayloadFlag, int16_t signal[]);
#endif /* ifndef DECODER_H */
