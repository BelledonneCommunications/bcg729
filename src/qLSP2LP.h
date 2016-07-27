/*
 qLSP2LP.h

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
#ifndef QLSP2LP_H
#define QLSP2LP_H
/*****************************************************************************/
/* qLSP2LP : convert qLSP into LP parameters according to spec. 3.2.6        */
/*    parameters:                                                            */
/*      -(i) qLSP : 10 LSP to be converted in Q0.15 range [-1, +1[           */
/*      -(o) LP : 10 LP coefficients in Q12                                  */
/*                                                                           */
/*****************************************************************************/
void qLSP2LP(word16_t qLSP[], word16_t LP[]);
#endif /* ifndef QLSP2LP_H */
