/*
 computeWeightedSpeech.h

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
#ifndef COMPUTEWEIGHTEDSPEECH
#define COMPUTEWEIGHTEDSPEECH

/*****************************************************************************/
/* computeWeightedSpeech: compute wieghted speech according to spec A3.3.3   */
/*    parameters:                                                            */
/*      -(i) inputSignal : 90 values buffer accessed in range [-10, 79] in Q0*/
/*      -(i) qLPCoefficients: 20 coefficients(10 for each subframe) in Q12   */
/*      -(i) weightedqLPCoefficients: 20 coefficients(10 for each subframe)  */
/*           in Q12                                                          */
/*      -(i/o) weightedInputSignal: 90 values in Q0: [-10, -1] as input      */
/*             [0,79] as output in Q0                                        */
/*      -(o) LPResidualSignal: 80 values of residual signal in Q0            */
/*                                                                           */
/*****************************************************************************/
void computeWeightedSpeech(word16_t inputSignal[], word16_t qLPCoefficients[], word16_t weightedqLPCoefficients[], word16_t weightedInputSignal[], word16_t LPResidualSignal[]);
#endif /* ifndef COMPUTEWEIGHTEDSPEECH */
