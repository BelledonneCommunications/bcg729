/*
 qLSP2LP.c

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

/* local function */
void computePolynomialCoefficients(word16_t qLSP[], word32_t f[]);

/*****************************************************************************/
/* qLSP2LP : convert qLSP into LP parameters according to spec. 3.2.6        */
/*    parameters:                                                            */
/*      -(i) qLSP : 10 LSP to be converted in Q0.15 range [-1, +1[           */
/*      -(o) LP : 10 LP coefficients in Q12                                  */
/*                                                                           */
/*****************************************************************************/
void qLSP2LP(word16_t qLSP[], word16_t LP[])
{
	int i;
	word32_t f1[6], f2[6]; /* define two buffer to store the polynomials coefficients (size is 6 for 5 coefficient because fx[0] is used during computation as a buffer) */
	/* pointers to access directly the elements 1 of f1 and f2 */
	word32_t *fp1=&(f1[1]);
	word32_t *fp2=&(f2[1]);
	computePolynomialCoefficients(qLSP, f1);

	computePolynomialCoefficients(&(qLSP[1]), f2);
	
	/* convert f coefficient in f' coefficient f'1[i] = f1[i]+f[i-1] and f'2[i] = f2[i] - f2[i-1] */
	for (i=5; i>0; i--) {
		f1[i] = ADD32(f1[i], f1[i-1]); /* f1 is still in Q24 */
		f2[i] = SUB32(f2[i], f2[i-1]); /* f1 is still in Q24 */
	}

	/* compute the LP coefficients */
	/* fx[0] is not needed anymore, acces the f1 and f2 buffers via the fp1 and fp2 pointers */
	for (i=0; i<5; i++) {
		/* i in [0,5[ : LP[i] = (f1[i] + f2[i])/2 */
		LP[i] = PSHR(ADD32(fp1[i], fp2[i]),13); /* f1 and f2 in Q24, LP in Q12 */
		/* i in [5,9[ : LP[i] = (f1[9-i] - f2[9-i])/2 */
		LP[9-i] = PSHR(SUB32(fp1[i], fp2[i]),13); /* f1 and f2 in Q24, LP in Q12 */
	}
	return;
}


/*****************************************************************************/
/* computePolynomialCoefficients : according to spec. 3.2.6                  */
/*    parameters:                                                            */
/*      -(i) qLSP : 10 LSP to be converted in Q0.15 range [-1, +1[           */
/*      -(o) f : 6 values in Q24 : polynomial coefficients on 32 bits        */
/*                                                                           */
/*****************************************************************************/
void computePolynomialCoefficients(word16_t qLSP[], word32_t f[])
{
	int i,j;
	
	/* init values */
	/* f[-1] which is not available is to be egal at 0, it is directly removed in the following when used as a factor */
	f[0] = 16777216; /* 1 in Q24, f[0] is used only for the other coefficient computation */

	/* correspont to i=1 in the algorithm description in spec. 3.2.6 */
	f[1] = MULT16_16(qLSP[0], -1024); /* f[1] = -2*qLSP[0], with qLSP in Q0.15 * -2*2^9(-1024) -> f[1] in Q1.24 */

	/* so we start with i=2 */
	/* Note : index of qLSP are -1 respect of what is in the spec because qLSP array is indexed from 0-9 and not 1-10 */
	for (i=2; i<6; i++) {
		/* spec: f[i] = 2(f[i-2] - qLSP[2i-1]*f[i-1]) */
		f[i] = SHL(SUB32(f[i-2], MULT16_32_P15(qLSP[2*i-2], f[i-1])),1); /* with qLSP in Q0.15 and f in Q24 */
		for (j=i-1; j>1; j--) { /* case of j=1 is just doing f[1] -= 2*qLSP[2i-1], done after the loop in order to avoid reference to f[-1] */
			/* spec: f[j] = f[j] -2*qLSP[2i-i]*f[j-1] + f[j-2] (all right terms refer to the value at the previous iteration on i indexed loop) */
			f[j] = ADD32(f[j], SUB32(f[j-2], MULT16_32_P14(qLSP[2*i-2], f[j-1]))); /* qLPS in Q0.15 and f in Q24, using MULT16_32_P14 instead of P15 does the *2 on qLSP. Result in Q24 */
		}
		/* f[1] -=  2*qLSP[2i-1] */
		f[1] = SUB32(f[1], SHL(qLSP[2*i-2],10)); /* qLSP in Q0.15, must be shift by 9 to get in Q24 and one more to be *2 */
	}
	return;
}
