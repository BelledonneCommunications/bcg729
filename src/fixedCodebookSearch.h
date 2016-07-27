/*
 fixedCodebookSearch.h

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
#ifndef FIXEDCODEBOOKSEARCH_H
#define FIXEDCODEBOOKSEARCH_H
/*****************************************************************************/
/* fixedCodebookSearch: compute fixed codebook parameters (codeword and sign)*/
/*      compute also fixed codebook vector as in spec 3.8.1                  */
/*    parameters:                                                            */
/*      -(i) targetSignal: 40 values as in spec A.3.6 in Q0                  */
/*      -(i) impulseResponse: 40 values as in spec A.3.5 in Q12              */
/*      -(i) intPitchDelay: current integer pitch delay                      */
/*      -(i) lastQuantizedAdaptativeCodebookGain: previous subframe pitch    */
/*           gain quantized in Q14                                           */
/*      -(i) filteredAdaptativeCodebookVector : 40 values in Q0              */
/*      -(i) adaptativeCodebookGain : in Q14                                 */
/*      -(o) fixedCodebookParameter                                          */
/*      -(o) fixedCodebookPulsesSigns                                        */
/*      -(o) fixedCodebookVector : 40 values as in spec 3.8, eq45 in Q13     */
/*      -(o) fixedCodebookVectorConvolved : 40 values as in spec 3.9, eq64   */
/*           in Q12.                                                         */
/*                                                                           */
/*****************************************************************************/
void fixedCodebookSearch(word16_t targetSignal[], word16_t impulseResponse[], int16_t intPitchDelay, word16_t lastQuantizedAdaptativeCodebookGain, word16_t filteredAdaptativeCodebookVector[], word16_t adaptativeCodebookGain,
			uint16_t *fixedCodebookParameter, uint16_t *fixedCodebookPulsesSigns, word16_t fixedCodebookVector[], word16_t fixedCodebookVectorConvolved[]);
#endif /* ifndef FIXEDCODEBOOKSEARCH_H */
