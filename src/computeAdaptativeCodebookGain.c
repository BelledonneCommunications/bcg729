/*
 computeAdaptativeCodebookGain.c

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

#include "typedef.h"
#include "codecParameters.h"
#include "basicOperationsMacros.h"
#include "utils.h"

#include "computeAdaptativeCodebookGain.h"

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
word16_t computeAdaptativeCodebookGain(word16_t targetSignal[], word16_t filteredAdaptativeCodebookVector[], word64_t *gainQuantizationXy, word64_t *gainQuantizationYy)
{
	int i;
	word32_t gain;

	*gainQuantizationXy = 0; /* contains the scalar product targetSignal, filteredAdaptativeCodebookVector : numerator */
	*gainQuantizationYy = 0; /* contains the scalar product filteredAdaptativeCodebookVector^2 : denominator */

	for (i=0; i<L_SUBFRAME; i++) {
		*gainQuantizationXy = MAC64(*gainQuantizationXy, targetSignal[i], filteredAdaptativeCodebookVector[i]);
		*gainQuantizationYy = MAC64(*gainQuantizationYy, filteredAdaptativeCodebookVector[i], filteredAdaptativeCodebookVector[i]);
	}

	/* check on values of xx and xy */
	if (*gainQuantizationXy<=0) { /* gain would be negative -> return 0 */
		/* this test covers the case of yy(denominator)==0 because if yy==0 then all y==0 and thus xy==0 */
		return 0;
	}

	/* output shall be in Q14 */
	gain = DIV64(SHL64(*gainQuantizationXy,14),*gainQuantizationYy); /* gain in Q14 */
	
	/* check if it is not above 1.2 */
	if (gain>ONE_POINT_2_IN_Q14) {
		gain = ONE_POINT_2_IN_Q14;
	}

	return (word16_t)gain;
}
