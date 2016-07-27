/*
 LSPQuantization.h

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
#ifndef LSPQUANTIZATION_H
#define LSPQUANTIZATION_H
void initLSPQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext);

/*****************************************************************************/
/* LSPQuantization : Convert LSP to LSF, Quantize LSF and find L parameters, */
/*      qLSF->qLSP as described in spec A3.2.4                               */
/*    parameters:                                                            */
/*      -(i/o) encoderChannelContext : the channel context data              */
/*      -(i) LSPCoefficients : 10 LSP coefficients in Q15                    */
/*      -(i) qLSPCoefficients : 10 qLSP coefficients in Q15                  */
/*      -(o) parameters : 4 parameters L0, L1, L2, L3                        */
/*                                                                           */
/*****************************************************************************/
void LSPQuantization(bcg729EncoderChannelContextStruct *encoderChannelContext, word16_t LSPCoefficients[], word16_t qLSPCoefficients[], uint16_t parameters[]);

/**********************************************************************************/
/* noiseLSPQuantization : Convert LSP to LSF, Quantize LSF and find L parameters, */
/*      qLSF->qLSP as described in spec A3.2.4                                    */
/*    parameters:                                                                 */
/*      -(i/o) previousqLSF : 4 previousqLSF, is updated by this function         */
/*      -(i) LSPCoefficients : 10 LSP coefficients in Q15                         */
/*      -(o) qLSPCoefficients : 10 qLSP coefficients in Q15                       */
/*      -(o) parameters : 3 parameters L0, L1, L2                                 */
/*                                                                                */
/**********************************************************************************/
void noiseLSPQuantization(word16_t previousqLSF[MA_MAX_K][NB_LSP_COEFF], word16_t LSPCoefficients[], word16_t qLSPCoefficients[], uint8_t parameters[]);
#endif /* LSPQUANTIZATION_H */
