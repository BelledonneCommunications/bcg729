/*
 computeLP.h

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
#ifndef COMPUTELP_H
#define COMPUTELP_H
/*****************************************************************************/
/* computeLP : As described in spec 3.2.1 and 3.2.2 : Windowing,             */
/*      Autocorrelation and Levinson-Durbin algorithm                        */
/*    parameters:                                                            */
/*      -(i) signal: 240 samples in Q0, the last 40 are from next frame      */
/*      -(o) LPCoefficientsQ12: 10 LP coefficients in Q12                    */
/*                                                                           */
/*****************************************************************************/
void computeLP(word16_t signal[], word16_t LPCoefficientsQ12[]);
#endif /* ifndef COMPUTELP_H */
