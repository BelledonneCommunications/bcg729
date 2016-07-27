/*
 LP2LSPConversion.h

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
#ifndef LP2LSPCONVERSION_H
#define LP2LSPCONVERSION_H
/*****************************************************************************/
/* LP2LSPConversion : Compute polynomials, find their roots as in spec A3.2.3*/
/*    parameters:                                                            */
/*      -(i) LPCoefficients[] : 10 coefficients in Q12                       */
/*      -(o) LSPCoefficients[] : 10 coefficients in Q15                      */
/*                                                                           */
/*    return value :                                                         */
/*      - boolean: 1 if all roots found, 0 if unable to compute 10 roots     */
/*                                                                           */
/*****************************************************************************/
int LP2LSPConversion(word16_t LPCoefficients[], word16_t LSPCoefficients[]);
#endif /* ifndef LP2LSPCONVERSION_H */
