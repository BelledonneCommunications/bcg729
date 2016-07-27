/*
 preProcessing.h

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
#ifndef PREPROCESSING_H
#define PREPROCESSING_H
/* internal variable initialisations                                         */
void initPreProcessing(bcg729EncoderChannelContextStruct *encoderChannelContext);

/*****************************************************************************/
/* preProcessing : 2nd order highpass filter with cut off frequency at 140Hz */
/*      Algorithm:                                                           */
/*      y[i] = BO*x[i] + B1*x[i-1] + B2*x[i-2] + A1*y[i-1] + A2*y[i-2]       */
/*    parameters :                                                           */
/*      -(i/o) encoderChannelContext : the channel context data              */
/*      -(i) signal : 80 values in Q0                                        */
/*      -(o) preProcessedSignal : 80 values in Q0                            */
/*                                                                           */
/*****************************************************************************/
void preProcessing(bcg729EncoderChannelContextStruct *encoderChannelContext, word16_t signal[], word16_t preProcessedSignal[]);
#endif /* ifndef PREPROCESSING_H */
