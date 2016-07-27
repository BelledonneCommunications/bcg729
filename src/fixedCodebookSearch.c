/*
 fixedCodebookSearch.c

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
#include <stdlib.h>

#include "fixedCodebookSearch.h"

/*** local functions ***/
void computeImpulseResponseCorrelationMatrix(word16_t impulseResponse[], word16_t correlationSignal[], int correlationSignalSign[], word32_t Phi[L_SUBFRAME][L_SUBFRAME]);
void computePhiDiagonal(int j, word16_t impulseResponse[], word32_t Phi[L_SUBFRAME][L_SUBFRAME], uint16_t PhiScaling);

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
			uint16_t *fixedCodebookParameter, uint16_t *fixedCodebookPulsesSigns, word16_t fixedCodebookVector[], word16_t fixedCodebookVectorConvolved[])
{
	int i,j,n;
	word16_t fixedCodebookTargetSignal[L_SUBFRAME];
	word32_t correlationSignal32[L_SUBFRAME]; /* on 32 bits in Q12 */ 
	word16_t correlationSignal[L_SUBFRAME]; /* normalised to fit on 13 bits */
	word32_t correlationSignalMax = 0;
	word32_t abscCrrelationSignal32;
	uint16_t correlationSignalMaxNorm;
	int correlationSignalSign[L_SUBFRAME]; /* to store the sign of each correlationSignal element */
	/* build the matrix Ф' : impulseResponse correlation matrix spec 3.8.1 eq51, eq56 and eq57 */
	/* correlationSignal turns to absolute values and sign of elements is stored in correlationSignalSign */
	word32_t Phi[L_SUBFRAME][L_SUBFRAME];
	int m3Base;
	int i0=0, i1=0, i2=0, i3=0;
	word32_t correlationSquareMax = -1;
	word32_t energyMax = 1;
	int m0=0, m1=0, m2=0, m3=0;
	int mSwitch[2][4] = {{2,3,0,1},{3,0,1,2}};
	int mIndex;
	int jx = 0;

	/* compute the target signal for fixed codebook spec 3.8.1 eq50 : fixedCodebookTargetSignal[i] = targetSignal[i] - (adaptativeCodebookGain * filteredAdaptativeCodebookVector[i]) */
	for (i=0; i<L_SUBFRAME; i++) {
		fixedCodebookTargetSignal[i] = MSU16_16_Q14(targetSignal[i], filteredAdaptativeCodebookVector[i], adaptativeCodebookGain); /* adaptativeCodebookGain in Q14, other values in Q0 */
	}

	/* update impulse vector as in spec 3.8 eq49 */
	for (i=intPitchDelay; i<L_SUBFRAME; i++) {
		impulseResponse[i] = MAC16_16_Q14(impulseResponse[i], impulseResponse[i-intPitchDelay], lastQuantizedAdaptativeCodebookGain); /* h[n] = h[n] + β*h[n-T], impulseResponse in Q12, lastQuantizedAdaptativeCodebookGain in Q14 */
	}

	/* compute the correlation signal as in spec 3.8.1 eq52 */
	/* compute on 32 bits and get the maximum */
	for (n=0; n<L_SUBFRAME; n++) {
		correlationSignal32[n] = 0;
		for (i=n; i<L_SUBFRAME; i++) {
			correlationSignal32[n] = MAC16_16(correlationSignal32[n], fixedCodebookTargetSignal[i], impulseResponse[i-n]);
		}
		abscCrrelationSignal32 = correlationSignal32[n]>=0?correlationSignal32[n]:-correlationSignal32[n];
		if (abscCrrelationSignal32>correlationSignalMax) {
			correlationSignalMax = abscCrrelationSignal32;
		}
	}
	/* normalise on 13 bits */
	correlationSignalMaxNorm = countLeadingZeros(correlationSignalMax);
	if (correlationSignalMaxNorm<18) { /* if it doesn't already fit on 13 bits */
		for (i=0; i<L_SUBFRAME; i++) {
			correlationSignal[i] = (word16_t)(SHR(correlationSignal32[i], 18-correlationSignalMaxNorm));
		 }
	} else { /* it fits on 13 bits, just copy it to the 16 bits buffer */
		for (i=0; i<L_SUBFRAME; i++) {
			correlationSignal[i] = (word16_t)correlationSignal32[i];
		}
	}

	computeImpulseResponseCorrelationMatrix(impulseResponse, correlationSignal, correlationSignalSign, Phi);
	
	/* search for impulses leading to a max in C^2/E : spec 3.8.1 eq53 */
	/* algorithm, not described in spec, retrieved from ITU code */
	/* by tracks are intended series of index m0 track is 0,5,10,...35. m1 is 1,6,11,..,36. m2 is 2,7,12,..,37. m3 is 3,8,13,..,38. m4 is 4,9,14,..,39 */
	/* note index m3 will follow track m3 and m4 */
	/* The following search is performed twice: first with m3 index following m3 track, and then m3 index follow m4 track */
	/*    The following operation are performed twice, first using indexes m2, m3, m0, m1 and then m3, m0, m1, m2 as following */
	/*       description is made for the first run, for the second one, juste substitute m2 by m3, m3 by m0, m0 by m1 and m1 by m2 */
	/*       - search in m2 track two maxima for the correlation Signal. For each of this maximum : */
	/*         -- compute for the whole m3 track (8 values) the values C^2 and E (see eq58 and 59) and keep the one giving the best ratio */
	/*       - compute for the whole tracks m0 and m1 (64 values) the values C^2 and E (keeping the m2 and m3 previously computed) and save the one giving the best ratio */
	for (m3Base=3; m3Base<5; m3Base++) {
		for(mIndex=0; mIndex<2; mIndex++) {
			/* define for this loop on m3 track the Correlation and Energy giving the maximum of eq53 */
			word32_t m3TrackCorrelationSquare = -1;
			word32_t m3TrackEnergy = 1;

			/* Loop on the two maxima of correlation in the m2 index */
			int firstM2 = 0; /* save the first maximum index to not select it again */
			word16_t correlationM2M3Max = 0; /* stores the contribution of m2 and m3 impulses to the correlation for the maximum selected */
			word32_t energyM2M3Max = 0; /* same thing but for the energy */
			for (i=0; i<2; i++) {
				word16_t correlationM2 = -1;
				int currentM2=0;
				word32_t energyM2;
				for (j=mSwitch[mIndex][0]; j<L_SUBFRAME; j+=5) { /* in the m2 range, find the correlation Max -> select m2 */
					if (correlationSignal[j]>correlationM2 && j!=firstM2) {
						currentM2 = j;
						correlationM2=correlationSignal[j];
					}
				}
				firstM2 = currentM2; /* to avoid selecting the same maximum at next iteration */

				energyM2 = Phi[currentM2][currentM2]; /* compute the energy with terms of eq55 using m2 only: Phi'(m2,m2) */		
			
				/* with selected m2, test the 8 m3 possibilities for the current m3 track */
				for (j=mSwitch[mIndex][1]; j<L_SUBFRAME; j+=5) {
					word16_t correlationM2M3 = ADD16(correlationM2, correlationSignal[j]); /* compute the correlation sum due to m2 and m3 pulses */
					word32_t energyM2M3 =  ADD32(energyM2, ADD32(Phi[currentM2][j], Phi[j][j])); /* compute the energy if eq55 using term including m2 and m3: Phi'(m2,m2) is already in energyM2 + Phi'(m2,m3) + Phi'(m3,m3) */
					word32_t correlationM2M3Square = MULT16_16(correlationM2M3, correlationM2M3);
					/* check if the current correlation/energy couple gives better results than the stored one : maximise C^2/E -> C^2/E > C^2max/Emax => Emax*C^2 > C^2max*E */
					if (MULT32_32(m3TrackEnergy,correlationM2M3Square) > MULT32_32(energyM2M3, m3TrackCorrelationSquare)) {
						m3TrackCorrelationSquare = correlationM2M3Square;
						m3TrackEnergy = energyM2M3;
						correlationM2M3Max = correlationM2M3;
						m3 = j;
						m2 = currentM2; 
					}
				}
			}
			energyM2M3Max = m3TrackEnergy;

			/* reset the current m3 track correlationSquare and energy */
			m3TrackCorrelationSquare = -1;
			m3TrackEnergy = 1;

			for (i=mSwitch[mIndex][2]; i<L_SUBFRAME; i+=5) { /* test the 8 possibilities for m0 track */
				word16_t correlationM2M3M0 = ADD16(correlationM2M3Max, correlationSignal[i]); /* compute correlation with current m0 taking in account the previously selected m2 and m3 */
				word32_t energyM2M3M0 = ADD32(energyM2M3Max, ADD32(Phi[i][i], ADD32(Phi[i][m2], Phi[i][m3]))); /* add to the previously computed energy the terms of eq59 we can compute with the selected m0: Phi'(m0,m0) + Phi'(m0,m2) + Phi'(m0,m3) */ 
				for (j=mSwitch[mIndex][3]; j<L_SUBFRAME; j+=5) { /* test the 8 possibilities for m1 track */
					word16_t correlationM2M3M0M1 = ADD16(correlationM2M3M0, correlationSignal[j]); /* compute correlation with current m1 taking in account the previously selected m2, m3 and m0 */
					word32_t energyM2M3M0M1 = ADD32(energyM2M3M0, ADD32(Phi[j][i], ADD32(Phi[j][j], ADD32(Phi[j][m2], Phi[j][m3])))); /* add to the previously computed energy the terms of eq59 we can compute with the selected m1: Phi'(m1,m0) + Phi'(m1,m1) + Phi'(m1,m2) + Phi'(m1,m3) */ 
					word32_t correlationM2M3M0M1Square = MULT16_16(correlationM2M3M0M1, correlationM2M3M0M1);
					/* check if the current correlation/energy couple gives better results than the stored one : maximise C^2/E -> C^2/E > C^2max/Emax => Emax*C^2 > C^2max*E */
					if (MULT32_32(m3TrackEnergy,correlationM2M3M0M1Square) > MULT32_32(energyM2M3M0M1, m3TrackCorrelationSquare)) {
						m3TrackCorrelationSquare = correlationM2M3M0M1Square;
						m3TrackEnergy = energyM2M3M0M1;
						m1 = j;
						m0 = i; 
					}
				}
			}

			/* check with currently selected indexes if this one is better */
			if (MULT32_32(energyMax,m3TrackCorrelationSquare) > MULT32_32(m3TrackEnergy, correlationSquareMax)) {
				correlationSquareMax = m3TrackCorrelationSquare;
				energyMax = m3TrackEnergy;
				if (mIndex==0) {
					i0 = m0;
					i1 = m1;
					i2 = m2;
					i3 = m3;
				} else {
					i0 = m3;
					i1 = m0;
					i2 = m1;
					i3 = m2;
				}
				jx = m3Base - 3; /* needed for parameter computation apec 3.8.2 eq62 */
			}

			
		}
		mSwitch[0][1]++; mSwitch[1][0]++; /*increment the m3Base into the mSwitch */
	}

	/* compute the fixedCodebookVector */
	for (i=0; i<L_SUBFRAME; i++) {
		fixedCodebookVector[i] = 0; /* reset the vector */
	}

	/* set the four pulses, in Q13 */
	fixedCodebookVector[i0] = SHL((word16_t)correlationSignalSign[i0], 13); 
	fixedCodebookVector[i1] = SHL((word16_t)correlationSignalSign[i1], 13); 
	fixedCodebookVector[i2] = SHL((word16_t)correlationSignalSign[i2], 13); 
	fixedCodebookVector[i3] = SHL((word16_t)correlationSignalSign[i3], 13); 

	/* adapt it according to eq48 */
	for (i=intPitchDelay; i<L_SUBFRAME; i++) {
		fixedCodebookVector[i] = MAC16_16_Q14(fixedCodebookVector[i], fixedCodebookVector[i-intPitchDelay], lastQuantizedAdaptativeCodebookGain); /* h[n] = h[n] + β*h[n-T], fixedCodebookVector in Q13, lastQuantizedAdaptativeCodebookGain in Q14 */
	}

	/* compute the parameters */
	*fixedCodebookParameter = (uint16_t)(	MULT16_16_Q15((word16_t)i0, O2_IN_Q15)
					+	((MULT16_16_Q15((word16_t)i1, O2_IN_Q15))<<3)
					+	((MULT16_16_Q15((word16_t)i2, O2_IN_Q15))<<6)
					+	((((MULT16_16_Q15((word16_t)i3, O2_IN_Q15))<<1)+jx)<<9)
				);
	*fixedCodebookPulsesSigns = (uint16_t)((correlationSignalSign[i0]+1)>>1) | /* as in spec 3.8.2 eq61 */
					(uint16_t)(((correlationSignalSign[i1]+1)>>1)<<1) |
					(uint16_t)(((correlationSignalSign[i2]+1)>>1)<<2) |
					(uint16_t)(((correlationSignalSign[i3]+1)>>1)<<3);
	
	/* compute the fixedCodebook vector convolved with impulse response spec 3.9 eq64 */
	/* this vector is used in gain quantization but computed here because it's faster doing it having directly the impulses positions */
	/* eq64 make use of fixedCodebook vector adapted by eq48, using the impulse position(and thus fixed codebook vector before the adaptation)  but */
	/* the impulse response adapted as in eq49 gives the same output */
	/* reset the vector */
	for (i=0; i<i0; i++) fixedCodebookVectorConvolved[i] = 0;

 	if(correlationSignalSign[i0] > 0) {
		for(i=i0, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = impulseResponse[j];
		}
	} else {
		for(i=i0, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = -impulseResponse[j];
		}
	}

 	if(correlationSignalSign[i1] > 0) {
		for(i=i1, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = ADD16(fixedCodebookVectorConvolved[i], impulseResponse[j]);
		}
	} else {
		for(i=i1, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = SUB16(fixedCodebookVectorConvolved[i], impulseResponse[j]);
		}
	}

 	if(correlationSignalSign[i2] > 0) {
		for(i=i2, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = ADD16(fixedCodebookVectorConvolved[i], impulseResponse[j]);
		}
	} else {
		for(i=i2, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = SUB16(fixedCodebookVectorConvolved[i], impulseResponse[j]);
		}
	}

 	if(correlationSignalSign[i3] > 0) {
		for(i=i3, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = ADD16(fixedCodebookVectorConvolved[i], impulseResponse[j]);
		}
	} else {
		for(i=i3, j=0; i<L_SUBFRAME; i++, j++) {
			fixedCodebookVectorConvolved[i] = SUB16(fixedCodebookVectorConvolved[i], impulseResponse[j]);
		}
	}

	return;
}


/*****************************************************************************/
/* computeImpulseResponseCorrelationMatrix: as in spec 3.8.1 eq51, eq56, eq57*/
/*    parameters:                                                            */
/*      -(i) impulseResponse : 40 values in Q12                              */
/*      -(i/o) correlationSignal: 40 values in Q12 get absolute value of     */
/*                input as output as specified in spec 3.8.1                 */
/*      -(o) correlationSignalSign : 40 values of -1 or 1 : the sign of      */
/*                the input correlationSignal elements                       */
/*                                                                           */
/*    return value: a triangular matrix composed of Phi(i,j) in Q24          */
/*         where Phi(i,j) = ∑h(n-i)*h(n-j) n in i..39                        */
/*         The matrix is then modified as decribed in eq56 and eq 57         */
/*                                                                           */
/*  Algorithm : the matrix elements are computed for j<=i                    */
/*     due to matrix element definition we have                              */
/*      Phi(i,j) = Phi(i+1,j+1) + h(39-i)*h(39-j)                            */
/*   - The matrix is computed starting from element Phi(39,x)=h(0)*h(39-x)   */
/*   and adding terms h(39-i)*h(39-j) give all the element of the diagonal   */
/*   up to Phi(39-x,0)                                                       */
/*   - Some diagonals are not computed because not needed :                  */
/*     Phi(39,34), Phi(39,29), Phi(39,24), .. Phi(39,4)                      */
/*   - The matrix elements signs are then adjusted according to eq56         */
/*   - The correlationSignal is modified to get absolute values of each      */
/*   element in it.                                                          */
/*   - Matrix elements are then duplicated to make access to them easier     */
/*                                                                           */
/*****************************************************************************/
void computeImpulseResponseCorrelationMatrix(word16_t impulseResponse[], word16_t correlationSignal[], int correlationSignalSign[], word32_t Phi[L_SUBFRAME][L_SUBFRAME])
{
	int i,j,iComp;
	word32_t acc = 0;
	uint16_t PhiScaling = 0;
	int correlationSignalSignInv[L_SUBFRAME];

	/* first compute the diagonal Phi(x,x) : Phi(39,39) = h[0]^2 # Phi(38,38) = Phi(39,39)+h[1]^2 */
	/* this diagonal must be divided by 2 according to spec 3.8.1 eq57 */
	for (i=0, iComp=L_SUBFRAME-1; i<L_SUBFRAME; i++, iComp--) { /* i in [0..39], iComp in [39..0] */
		acc = MAC16_16(acc, impulseResponse[i], impulseResponse[i]); /* impulseResponse in Q12 -> acc in Q24 */
		Phi[iComp][iComp] = SHR(acc,1); /* divide by 2: eq57*/
	}

	/* check for possible overflow: Phi will be summed 10 times, so max Phi (by construction Phi[0][0]*2 is the max of Phi-> 2*Phi[0][0]*10 must be < 0x7fff ffff -> Phi[0][0]< 0x06666666 - otherwise scale Phi)*/
	if (Phi[0][0]>0x6666666) {
		PhiScaling = 3 - countLeadingZeros((Phi[0][0]<<1) + 0x3333333); /* complement 0xccccccc adding 0x3333333 to shift by one when max(2*Phi[0][0]) is in 0x0fffffff < max < 0xcccccc */
		for (i=0; i<L_SUBFRAME; i++) {
			Phi[i][i] = SHR(Phi[i][i],PhiScaling);
		}
	}
	
	/* Compute all diagonals but the 34, 29, 24, 19, 14, 9 and 4*/
	for (i=0; i<8; i++) {
		for (j=0; j<4; j++) {
			computePhiDiagonal(5*i+j, impulseResponse, Phi, PhiScaling);
		}
	}

	/* correlationSignal -> absolute value and get sign (and his inverse in an array) */
	for (i=0; i<L_SUBFRAME; i++) {
		if (correlationSignal[i] >= 0) {
			correlationSignalSign[i] = 1;
			correlationSignalSignInv[i] = -1;
		} else { /* correlationSignal < 0 */
			correlationSignalSign[i] = -1;
			correlationSignalSignInv[i] = 1;
			correlationSignal[i] = -correlationSignal[i];
		}
	}

	/* modify the signs according to eq56 */
	for (i=0; i<L_SUBFRAME; i++) {
		int *signOfCorrelationSignalJ;
		
		if (correlationSignalSign[i]>0) { /* is sign(correlationSignal[i]) is positive, use the correlationSignalSign otherwise the inverted one */
			signOfCorrelationSignalJ = correlationSignalSign;
		} else {
			signOfCorrelationSignalJ = correlationSignalSignInv;
		}
		for (j=0; j<=i; j++) { /* multiply by the selected sign the matrix element */
			/* Note : even the not needed and thus not computed elements are multiplicated... might found other way to do this sign stuff to be more efficient */
			Phi[i][j] =  Phi[i][j] * signOfCorrelationSignalJ[j];
		}
	}
	
	/* duplicate the usefull values to their symetric part to get easier acces to the matrix elements */
	for (i=0; i<8; i++) {
		for (j=0; j<4; j++) {
			int k;
			int startIndex = 5*i+j;
			for(k=0; k<=startIndex; k++) {
				Phi[startIndex-k][L_SUBFRAME-1-k] = Phi[L_SUBFRAME-1-k][startIndex-k];
			}
		}
	}
	return;
}

/* compute a diagonal of Phi values: start from Phi(39,j) and step Phi(38, j-1) down to Phi(39-j, 0) */
/*      Phi(i,j) = Phi(i+1,j+1) + h(39-i)*h(39-j)                            */
void computePhiDiagonal(int j, word16_t impulseResponse[], word32_t Phi[L_SUBFRAME][L_SUBFRAME], uint16_t PhiScaling)
{
	word32_t acc = 0;
	int i=L_SUBFRAME -1;
	int iComp = 0;
	int jComp = L_SUBFRAME - 1 - j;

	if (PhiScaling == 0) {
		for (; j>=0; j--, i--, jComp++, iComp++) {
			acc = MAC16_16(acc, impulseResponse[iComp], impulseResponse[jComp]);
			Phi[i][j] = acc;
		}
	} else {
		for (; j>=0; j--, i--, jComp++, iComp++) {
			acc = MAC16_16(acc, impulseResponse[iComp], impulseResponse[jComp]);
			Phi[i][j] = SHR(acc,PhiScaling);
		}
	}
}
