/*
 LP2LSPConversion.c

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

#include "LP2LSPConversion.h"

/* local functions and codebook */
word32_t ChebyshevPolynomial(word16_t x, word32_t f[]); /* return value in Q24 */
static const word16_t cosW0pi[NB_COMPUTED_VALUES_CHEBYSHEV_POLYNOMIAL]; /* cos(w) from 0 to Pi in 50 steps */

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
int LP2LSPConversion(word16_t LPCoefficients[], word16_t LSPCoefficients[])
{
	uint8_t i;
	word32_t f1[6];
	word32_t f2[6]; /* coefficients for polynomials F1 anf F2 in Q12 for computation, then converted in Q15 for the Chebyshev Polynomial function */
	uint8_t numberOfRootFound = 0; /* used to check the final number of roots found and exit the loop on each polynomial computation when we have 10 roots */
	word32_t *polynomialCoefficients;
	word32_t previousCx;
	word32_t Cx; /* value of Chebyshev Polynomial at current point in Q15 */

	/*** Compute the polynomials coefficients according to spec 3.2.3 eq15 ***/
	f1[0] = ONE_IN_Q12; /* values 0 are not part of the output, they are just used for computation purpose */ 
	f2[0] = ONE_IN_Q12;
	/* for (i = 0; i< 5; i++) {		*/
	/*   f1[i+1] = a[i+1] + a[10-i] - f1[i];	*/
	/*   f2[i+1] = a[i+1] - a[10-i] + f2[i];	*/
 	/* }					*/
	for (i=0; i<5; i++) {
		f1[i+1] = ADD32(LPCoefficients[i], SUB32(LPCoefficients[9-i], f1[i])); /* note: index on LPCoefficients are -1 respect to spec because the unused value 0 is not stored */
		f2[i+1] = ADD32(f2[i], SUB32(LPCoefficients[i], LPCoefficients[9-i])); /* note: index on LPCoefficients are -1 respect to spec because the unused value 0 is not stored */
	}
	/* convert the coefficients from Q12 to Q15 to be used by the Chebyshev Polynomial function (f1/2[0] aren't used so they are not converted) */
	for (i=1; i<6; i++) {
		f1[i] = SHL(f1[i], 3);
		f2[i] = SHL(f2[i], 3);
	}

	/*** Compute at each step(50 steps for the AnnexA version) the Chebyshev polynomial to find the 10 roots ***/
	/* start using f1 polynomials coefficients and altern with f2 after founding each root (spec 3.2.3 eq13 and eq14) */
	polynomialCoefficients = f1; /* start with f1 coefficients */
	previousCx = ChebyshevPolynomial(cosW0pi[0], polynomialCoefficients); /* compute the first point and store it as the previous value for polynomial */

	for (i=1; i<NB_COMPUTED_VALUES_CHEBYSHEV_POLYNOMIAL; i++) {
		Cx =  ChebyshevPolynomial(cosW0pi[i], polynomialCoefficients);
		if ((previousCx^Cx)&0x10000000) { /* check signe change by XOR on the value of first bit */
			/* divide 2 times the interval to find a more accurate root */
			uint8_t j;
			word16_t xLow = cosW0pi[i-1];
			word16_t xHigh = cosW0pi[i];
			word16_t xMean;
			for (j=0; j<2; j++) {
				word32_t middleCx;
				xMean = (word16_t)SHR(ADD32(xLow, xHigh), 1);
				middleCx = ChebyshevPolynomial(xMean, polynomialCoefficients); /* compute the polynome for the value in the middle of current interval */
				
				if ((previousCx^middleCx)&0x10000000) { /* check signe change by XOR on the value of first bit */
					xHigh = xMean;
					Cx = middleCx; /* used for linear interpolation on root */
				} else {
					xLow = xMean;
					previousCx = middleCx;
				}
			}

			/* toggle the polynomial coefficients in use between f1 and f2 */
			if (polynomialCoefficients==f1) {
				polynomialCoefficients = f2;
			} else {
				polynomialCoefficients = f1;
			}

			/* linear interpolation for better root accuracy */
			/* xMean = xLow - (xHigh-xLow)* previousCx/(Cx-previousCx); */
			xMean = (word16_t)SUB32(xLow, MULT16_32_Q15(SUB32(xHigh, xLow), DIV32(SHL(previousCx, 14), SHR(SUB32(Cx, previousCx), 1)))); /* Cx are in Q2.15 so we can shift them left 14 bits, the denominator is shifted righ by 1 so the division result is in Q15 */

			/* recompute previousCx with the new coefficients */
			previousCx = ChebyshevPolynomial(xMean, polynomialCoefficients);

			LSPCoefficients[numberOfRootFound] = xMean;

			numberOfRootFound++;
			if (numberOfRootFound == NB_LSP_COEFF) break; /* exit the for loop as soon as we habe all the LSP*/
		}
		
	}
	if (numberOfRootFound != NB_LSP_COEFF) return 0; /* we were not able to find the 10 roots */
	
	
	return 1;
}

/*****************************************************************************/
/* ChebyshevPolynomial : Compute the Chebyshev polynomial, spec 3.2.3 eq17   */
/*    parameters:                                                            */
/*      -(i) x : input value of polynomial function in Q15                   */
/*      -(i) f : the polynome coefficients, 6 values in Q15 on 32 bits       */
/*           f[0] is not used                                                */
/*    return value :                                                         */
/*      - result of polynomial function in Q15                               */
/*                                                                           */
/*****************************************************************************/
word32_t ChebyshevPolynomial(word16_t x, word32_t f[])
{
	/* bk in Q15*/
	word32_t bk; 
	word32_t bk1 = ADD32(SHL(x,1), f[1]); /* init: b4=2x+f1 */
	word32_t bk2 = ONE_IN_Q15; /* init: b5=1 */
	uint8_t k;
	for (k=3; k>0; k--) { /* at the end of loop execution we have b1 in bk1 and b2 in bk2 */ 
		bk = SUB32(ADD32(SHL(MULT16_32_Q15(x,bk1), 1), f[5-k]), bk2); /* bk = 2*x*bk1 − bk2 + f(5-k) all in Q15*/
		bk2 = bk1;
		bk1 = bk;
	}

	return SUB32(ADD32(MULT16_32_Q15(x,bk1), SHR(f[5],1)), bk2); /* C(x) = x*b1 - b2 + f(5)/2 */
}

/*****************************************************************************/
/*                                                                           */
/*  Codebook:                                                                */
/*                                                                           */
/*      x = cos(w) with w in [0,Pi] in 50 steps                              */
/*                                                                           */
/*****************************************************************************/
static const word16_t cosW0pi[NB_COMPUTED_VALUES_CHEBYSHEV_POLYNOMIAL] = { /* in Q15 */
     32760,     32703,     32509,     32187,     31738,     31164,
     30466,     29649,     28714,     27666,     26509,     25248,
     23886,     22431,     20887,     19260,     17557,     15786,
     13951,     12062,     10125,      8149,      6140,      4106,
      2057,         0,     -2057,     -4106,     -6140,     -8149,
    -10125,    -12062,    -13951,    -15786,    -17557,    -19260,
    -20887,    -22431,    -23886,    -25248,    -26509,    -27666,
    -28714,    -29649,    -30466,    -31164,    -31738,    -32187,
    -32509,    -32703,    -32760};
