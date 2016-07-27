/*
 postProcessing.h

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
#ifndef POSTPROCESSING_H
#define POSTPROCESSING_H
void initPostProcessing(bcg729DecoderChannelContextStruct *decoderChannelContext);
/*****************************************************************************/
/* postProcessing : high pass filtering and upscaling Spec 4.2.5             */
/*      Algorithm:                                                           */
/*      y[i] = BO*x[i] + B1*x[i-1] + B2*x[i-2] + A1*y[i-1] + A2*y[i-2]       */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i/o) signal : 40 values in Q0, reconstructed speech, output        */
/*             replaces the input in buffer                                  */
/*                                                                           */
/*****************************************************************************/
void postProcessing(bcg729DecoderChannelContextStruct *decoderChannelContext, word16_t signal[]);
#endif /* ifndef POSTPROCESSING_H */
