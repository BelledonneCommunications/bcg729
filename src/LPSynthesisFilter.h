/*
 LPSynthesisFilter.h

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
#ifndef LPSYNTHESISFILTER_H
#define LPSYNTHESISFILTER_H
/*****************************************************************************/
/* LPSynthesisFilter : as decribed in spec 4.1.6 eq77                        */
/*    parameters:                                                            */
/*      -(i) excitationVector: u(n), the excitation, 40 values in Q0         */
/*      -(i) LPCoefficients: 10 LP coefficients in Q12                       */
/*      -(i/o) recontructedSpeech: 50 values in Q0                           */
/*             [-NB_LSP_COEFF, -1] of previous values as input               */
/*             [0, L_SUBFRAME[ as output                                     */
/*                                                                           */
/*****************************************************************************/
void LPSynthesisFilter (word16_t *excitationVector, word16_t *LPCoefficients, word16_t *reconstructedSpeech);
#endif /* ifndef LPSYNTHESISFILTER_H */
