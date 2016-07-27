/*
 adaptativeCodebookSearch.h

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
#ifndef ADAPTATIVECODEBOOKSEARCH_H
#define ADAPTATIVECODEBOOKSEARCH_H
/*****************************************************************************/
/* adaptativeCodebookSearch: compute parameter P1 and P2 as in spec A.3.7    */
/*      compute also adaptative codebook vector as in spec 3.7.1             */
/*    parameters:                                                            */
/*      -(i/o) excitationVector: [-154,0[ previous excitation as input       */
/*                  Range [0,39[                                             */
/*                  40 words of LPResidualSignal as substitute for current   */
/*                  excitation (spec A.3.7) as input                         */
/*                  40 words of adaptative codebook vector in Q0 as output   */
/*                  Buffer in Q0 accessed in range [-154, 39]                */
/*      -(i/o) intPitchDelayMin: low boundary for pitch delay search         */
/*      -(i/o) intPitchDelayMax: low boundary for pitch delay search         */
/*                  Boundaries are updated during first subframe search      */
/*      -(i) impulseResponse: 40 values as in spec A.3.5 in Q12              */
/*      -(i) targetSignal: 40 values as in spec A.3.6 in Q0                  */
/*                                                                           */
/*      -(o) intPitchDelay: the integer pitch delay                          */
/*      -(o) fracPitchDelay: the fractionnal part of pitch delay             */
/*      -(o) pitchDelayCodeword: P1 or P2 codeword as in spec 3.7.2          */
/*      -(o) adaptativeCodebookVector: 40 words of adaptative codebook vector*/
/*             as described in spec 3.7.1, in Q0.                            */
/*      -(i) subFrameIndex: 0 for the first subframe, 40 for the second      */
/*                                                                           */
/*****************************************************************************/
void adaptativeCodebookSearch(word16_t excitationVector[], int16_t *intPitchDelayMin, int16_t *intPitchDelayMax, word16_t impulseResponse[], word16_t targetSignal[],
				int16_t *intPitchDelay, int16_t *fracPitchDelay, uint16_t *pitchDelayCodeword,  uint16_t subFrameIndex);
#endif /* ifndef ADAPTATIVECODEBOOKSEARCH_H */
