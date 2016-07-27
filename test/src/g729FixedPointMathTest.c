/*
 g729FixedPointMathTest.c

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
/*****************************************************************************/
/*                                                                           */
/* Test Program g729 Fixed Point Math library                                */
/*                                                                           */
/*     library provides:                                                     */
/*       g729Log2_Q0Q16  : Logarithm base 2                                  */
/*       g729Exp2_Q11Q16 : Exponentiel base 2                                */
/*       g729Sqrt_Q0Q7   : Square Root                                       */
/*       g729Cos_Q13Q15  : Cosine                                            */
/*       g729Atan_Q15Q13 : Arc Tangent                                       */
/*       g729Asin_Q15Q13 : Arc Sine                                          */
/*       g729Acos_Q15Q13 : Arc Cosine                                        */
/*                                                                           */
/*       Extention QxxQyy stands for input in Qxx output in Qyy              */
/*                                                                           */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "g729FixedPointMath.h"
#include "math.h"

#include "typedef.h"
#include "codecParameters.h"

/* convert the float result from math.h to a fixed point integer in Qxx */
word32_t float2FixedPoint(float x, int Q)
{
	return (word32_t)round((double)x*(double)(1<<Q));
}

/* convert a Qxx integer to float */
float fixedPoint2Float(word32_t x, int Q)
{
	return (float)x/(float)(1<<Q);
}

/* no arguments just test all the functions */
int main(int argc, char *argv[] )
{
	int64_t i;
	
	int computationNumber = 0;
	double diffSum = 0;
	double diffSumPercent = 0;
	int maxDiff = 0;
	int maxDiffIndex = -1;
	double maxDiffPercent = 0;
	int maxDiffIndexPercent = -1;

	/*       g729Log2_Q0Q16  : Logarithm base 2                                  */
	for (i=1; i<0x7FFFFFF; i+=0x10) {
		computationNumber++;
		word32_t resg729 = g729Log2_Q0Q16((word32_t)i);
		word32_t resMath = float2FixedPoint(log2f(i), 16);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Log2 : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);

	/*       g729Exp2_Q11Q16 : Exponentiel base 2                                */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=0; i<2048; i+=0x1) {
		computationNumber++;
		word32_t resg729 = g729Exp2_Q11Q16((word32_t)i);
		word32_t resMath = float2FixedPoint(exp2f(fixedPoint2Float(i,11)), 16);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Exp2 : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);


	/*       g729Sqrt_Q0Q7   : Square Root                                       */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=0; i<0x7FFFFFF; i+=0x8) {
		computationNumber++;
		word32_t resg729 = g729Sqrt_Q0Q7((word32_t)i);
		word32_t resMath = float2FixedPoint(sqrtf(i), 7);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Square Root : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);

	/*       g729InvSqrt_Q0Q30   : Inverse Square Root                    */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=1; i<0x7FFFFFF; i+=0x8) {
		computationNumber++;
		word32_t resg729 = g729InvSqrt_Q0Q31((word32_t)i)>>1;
		word32_t resMath = float2FixedPoint(1.0/sqrtf(i), 30);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Inverse Square Root : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);

	/*       g729Cos_Q13Q15  : Cosine                                            */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=0; i<25736; i+=0x1) {
		computationNumber++;
		word32_t resg729 = g729Cos_Q13Q15((word32_t)i);
		word32_t resMath = float2FixedPoint(cosf(fixedPoint2Float(i,13)), 15);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Cosine : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);

	/*       g729Atan_Q15Q13 : Arc Tangent                                       */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=-0x8000000; i<0x7ffffff; i+=0x1) {
		computationNumber++;
		word32_t resg729 = g729Atan_Q15Q13((word32_t)i);
		word32_t resMath = float2FixedPoint(atanf(fixedPoint2Float(i,15)), 13);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Arc Tangent : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);


	/*       g729Asin_Q15Q13 : Arc Sine                                          */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=-0x7fff; i<0x7ffe; i+=0x1) {
		computationNumber++;
		word32_t resg729 = g729Asin_Q15Q13((word32_t)i);
		word32_t resMath = float2FixedPoint(asinf(fixedPoint2Float(i,15)), 13);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Arc Sine : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);



	/*       g729Acos_Q15Q13 : Arc Cosine                                        */
	computationNumber = 0;
	diffSum = 0;
	diffSumPercent = 0;
	maxDiff = 0;
	maxDiffIndex = -1;
	maxDiffPercent = 0;
	maxDiffIndexPercent = -1;

	for (i=-0x7fff; i<0x7ffe; i+=0x1) {
		computationNumber++;
		word32_t resg729 = g729Acos_Q15Q13((word32_t)i);
		word32_t resMath = float2FixedPoint(acosf(fixedPoint2Float(i,15)), 13);
		int currentDiff = abs(resg729-resMath);
		double currentDiffPercent = 0;
		if (resMath!=0 && resg729!=0 && currentDiff!=0) currentDiffPercent = 100.0*currentDiff/(float)resMath;
		diffSum += currentDiff;
		diffSumPercent += currentDiffPercent;
		if (currentDiff>maxDiff) {
			maxDiff = currentDiff;
			maxDiffIndex = i;
		}	
		if (currentDiffPercent>maxDiffPercent) {
			maxDiffPercent = currentDiffPercent;
			maxDiffIndexPercent = i;
		}	
	}
	printf ("Arc Cosine : %d computation\n     Mean Diff %f Mean Percent Diff %f\n     Max Diff %d(%d) Max Percent Diff %f(%d)\n",
		computationNumber, diffSum/computationNumber, diffSumPercent/computationNumber, maxDiff, maxDiffIndex, maxDiffPercent, maxDiffIndexPercent);




exit (0);
}
