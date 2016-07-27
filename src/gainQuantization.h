/*
 gainQuantization.h

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
#ifndef GAINQUANTIZATION_H
#define GAINQUANTIZATION_H
void initGainQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext);
/*****************************************************************************/
/* gainQuantization : compute quantized adaptative and fixed codebooks gains */
/*      spec 3.9                                                             */
/*    parameters:                                                            */
/*      -(i/o) encoderChannelContext : the channel context data              */
/*      -(i) targetSignal: 40 values in Q0, x in eq63                        */
/*      -(i) filteredAdaptativeCodebookVector: 40 values in Q0, y in eq63    */
/*      -(i) convolvedFixedCodebookVector: 40 values in Q12, z in eq63       */
/*      -(i) fixedCodebookVector: 40 values in Q13                           */
/*      -(i) xy in Q0 on 64 bits term of eq63 computed previously            */
/*      -(i) yy in Q0 on 64 bits term of eq63 computed previously            */
/*      -(o) quantizedAdaptativeCodebookGain : in Q14                        */
/*      -(o) quantizedFixedCodebookGain : in Q1                              */
/*      -(o) gainCodebookStage1 : GA parameter value (3 bits)                */
/*      -(o) gainCodebookStage2 : GB parameter value (4 bits)                */
/*                                                                           */
/*****************************************************************************/
void gainQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext, word16_t targetSignal[], word16_t filteredAdaptativeCodebookVector[], word16_t convolvedFixedCodebookVector[], word16_t fixedCodebookVector[], word64_t Xy64, word64_t Yy64,
					word16_t *quantizedAdaptativeCodebookGain, word16_t *quantizedFixedCodebookGain, uint16_t *gainCodebookStage1, uint16_t *gainCodebookStage2);

#endif /* ifndef GAINQUANTIZATION_H */
