/*
 computeAdaptativeCodebookGain.h

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
#ifndef COMPUTEADAPTATIVECODEBOOKGAIN_H
#define COMPUTEADAPTATIVECODEBOOKGAIN_H

/*****************************************************************************/
/* computeAdaptativeCodebookGain: compute gain, return result in Q14         */
/*      in range [0,1.2]. Spec 3.7.3                                         */
/*    parameters:                                                            */
/*      -(i) targetSignal: 40 values in Q0                                   */
/*      -(i) filteredAdaptativeCodebookVector: 40 values in Q0               */
/*      -(o) gainQuantizationXy in Q0 on 64 bits used for gain quantization  */
/*      -(o) gainQuantizationYy in Q0 on 64 bits used for gain quantization  */
/*    return value:                                                          */
/*      - the adaptative codebook gain on 16 bits in Q14                     */
/*                                                                           */
/*****************************************************************************/
word16_t computeAdaptativeCodebookGain(word16_t targetSignal[], word16_t filteredAdaptativeCodebookVector[], word64_t *gainQuantizationXy, word64_t *gainQuantizationYy);
#endif /* ifndef COMPUTEADAPTATIVECODEBOOKGAIN_H */
