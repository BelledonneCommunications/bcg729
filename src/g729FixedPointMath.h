/*
 g729FixedPointMath.h

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
#ifndef G729FIXEDPOINTMATH_H
#define G729FIXEDPOINTMATH_H

/*****************************************************************************/
/*                                                                           */
/*  This library provides the following functions                            */
/*                                                                           */
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

#include "typedef.h"
#include "basicOperationsMacros.h"
#include "utils.h"

/* constants defined in Q16: actual values:
 KL0 = -2.059978
 KL1 = 5.770780
 KL2 = -3.847187 
 KL3 = 1.139907    
*/
#define KL0 -135003
#define KL1 378194
#define KL2 -252129
#define KL3 74705
/*****************************************************************************/
/* g729Log2_Q0Q16 : logarithm base 2, frac part computed from Taylor serie   */
/*    paremeters:                                                            */
/*      -(i) x : 32 bits integer in Q0, expected to be>0(not checked here)   */
/*    return value:                                                          */
/*      - the log2(x) in Q16 on 32 bits                                      */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word32_t g729Log2_Q0Q16(word32_t x)
{
	/* first get the integer part and put it in the 16 MSB of return value (in Q16) */
	uint16_t leadingZeros = countLeadingZeros(x); /* note: MSB is excluded as considered as sign bit */
	word32_t retValue = SHL32(SUB16(30,leadingZeros), 16);

	/* now shift the number to have it on this form 01XX XXXX XXXX XXXX, and keep only 16 bits -> consider it as a number in range [0.5, 1[ in Q0.15 */
	word16_t acc = (word16_t)VSHR32(x, 16-leadingZeros); 
	/* So calling int the integer part of the log2, we have:   */
	/* int = 30 - leadingZeros                                 */
	/* acc = x*2^(leadingZeros - 16)                           */
	/* acc = x*2^(14 - int)                                    */
	/* considering the content of acc as a Q15 number eq *2^-15*/
	/* acc = x*2^(14 -int)*2^-15                               */
	/* acc = x*2^(-1 -int)                                     */
	/* log2(acc) = log2(x) -1 - int                            */
	/* log2(x) ~= -3.059978 + 5.770780*x - 3.847187*x^2 + 1.139907*x^3 (for .5 < x < 1) Taylor Serie log2(x) at x near 0.75 */
	/* log2(x) + 1 = -2.059978 + x*(5.770780 +x(-3.847187 + 1.139907*x)) */
	/* with coeff in Q16 : */
	/* log2(acc) +1 = log2(x) - int =                           */
	/* log2(acc) +1 = -135003 +acc*(378194 +acc*(-252129 + acc*74705)) acc in Q15 and constants in Q16 -> final result will be log2(x) -int in Q16(on 32 bits) */
	word32_t acc32 = ADD32(KL0, MULT16_32_Q15(acc, ADD32(KL1, MULT16_32_Q15(acc, ADD32(KL2, MULT16_32_Q15(acc, KL3))))));
	
	return ADD32(retValue,acc32);
}

/* constants defined in Q15: actual values:
 E0 = 1
 E1 = log(2)
 E2 = 3-4*log(2)
 E3 = 3*log(2) - 2
*/
#define E0 16384
#define E1 11356
#define E2 3726
#define E3 1301
/*****************************************************************************/
/* g729Exp2_Q11Q16 : Exponentielle base 2                                    */
/*    paremeters:                                                            */
/*      -(i) x : 16 bits integer in Q11                                      */
/*    return value:                                                          */
/*      - exp2(x) in Q16 on 32 bits                                          */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word32_t g729Exp2_Q11Q16(word16_t x)
{
	int integer;
	word16_t frac;
	integer = SHR16(x,11);
	if (integer>14) {
		return 0x7fffffff;
	} else {
		if (integer < -15) {
			return 0;
		}
	}
	frac = SHL16(x-SHL16(integer,11),3);
	frac = ADD16(E0, MULT16_16_Q14(frac, ADD16(E1, MULT16_16_Q14(frac, ADD16(E2 , MULT16_16_Q14(E3,frac))))));
	return VSHR32(EXTEND32(frac), -integer-2);
}

/* constants in Q14 */
#define C0 3634
#define C1 21173
#define C2 -12627
#define C3 4204
/*****************************************************************************/
/* g729Sqrt_Q0Q7 : Square root                                               */
/*    paremeters:                                                            */
/*      -(i) x : 32 bits unsigned integer in Q0                              */
/*    return value:                                                          */
/*      - sqrt(x) in Q7 on 32 bits signed integer                            */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word32_t g729Sqrt_Q0Q7(uword32_t x)
{
	int k;
	word32_t rt;

	if (x==0) return 0;
	/* set x in Q14 in range [0.25,1[ */
	k = (19-unsignedCountLeadingZeros(x))>>1;
	x = VSHR32(x, (k<<1)); /* x = x.2^-2k */

	/* sqrt(x) ~= 0.22178 + 1.29227*x - 0.77070*x^2 + 0.25659*x^3 (for .25 < x < 1) */
	/* consider x as in Q14: y = x.2^(-2k-14) -> and give sqrt(y).2^14 = sqrt(x).2^(-k-7).2^14 */
	rt = ADD16(C0, MULT16_16_Q14(x, ADD16(C1, MULT16_16_Q14(x, ADD16(C2, MULT16_16_Q14(x, (C3))))))); /* rt = sqrt(x).2^(7-k)*/ 
	rt = VSHR32(rt,-k); /* rt = sqrt(x).2^7 */
	return rt;
}

/*****************************************************************************/
/* g729InvSqrt_Q0Q31 : Inverse Square root(1/Sqrt(x)                         */
/*      x is not tested to be >=1, shall be done by caller function          */
/*    paremeters:                                                            */
/*      -(i) x : 32 bits integer in Q0 in range [1, MAXINT32]                */
/*    return value:                                                          */
/*      - 1/sqrt(x) in Q31 on 32 bits in range [43341/2^31, MAXINT32]        */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word32_t g729InvSqrt_Q0Q31(word32_t x)
{
	if (x==1) return MAXINT32;
	return (word32_t)(DIV32_32_Q24(g729Sqrt_Q0Q7(x),x)); /* sqrt(x) in Q7 + Q24 -> Q31 */
}

/* constants Q0.15 */
#define Kcos1 32768 
#define Kcos2 -16384
#define Kcos3 1365 
#define Kcos4 -46

#define Ksin1 32768 
#define Ksin2 -5461
#define Ksin3 273 
#define Ksin4 -7

/*****************************************************************************/
/* g729Cos_Q13Q15 : Cosine fonction in [0, Pi]                               */
/*      x is not tested to be in correct range                               */
/*    paremeters:                                                            */
/*      -(i) x : 16 bits integer in Q13 in range [0, Pi(25736)]              */
/*    return value:                                                          */
/*      - cos(x) in Q0.15 on 16 bits in range [-1, 1[                        */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word16_t g729Cos_Q13Q15(word16_t x)
{
	/* input var x in Q2.13 and in ]0, Pi[ */
	word16_t x2,xScaled; 
	if (x<12868) {
		if (x<6434) { /* x in ]0, Pi/4[ */
	   		x2 = MULT16_16_P11(x,x); /* in Q0.15 */
			return SATURATE(ADD32(Kcos1, MULT16_16_P15(x2, ADD32(Kcos2, MULT16_16_P15(x2, ADD32(Kcos3, MULT16_16_P15(Kcos4, x2)))))), MAXINT16); /* return cos x, must saturate if return value is +1 */
		} else {/* x in [Pi/4, Pi/2[ */ 
			x = SUB16(12868,x); /* x = pi/2 -x, x in [0, Pi/4] in Q0.13 */ 
   			x2 = MULT16_16_P11(x,x); /* in Q0.15 */
			return (MULT16_16_P13(x,ADD32(Ksin1, MULT16_16_P15(x2, ADD32(Ksin2, MULT16_16_P15(x2, ADD32(Ksin3, MULT16_16_P15(Ksin4, x2)))))))); /* return cos x as sin(pi/2 -x) */
		}
	} else { /* x in [Pi/2, Pi[ */
		xScaled = SUB16(25736,x); /* xScaled = Pi - x -> in [0,Pi/2] with cos(Pi-x) = -cos(x) and sin(Pi-x) =  */
		if (x<19302) { /* x in [Pi/2, 3Pi/4], xScaled in [Pi/4, Pi/2] */
			xScaled = SUB16(12868,xScaled); /* xScaled = pi/2 - xScaled = x - Pi/2, xScaled in [0, Pi/4] in Q0.13 */
			x2 = MULT16_16_P11(xScaled,xScaled); /* in Q0.15 */
			return (MULT16_16_P13(-xScaled,ADD32(Ksin1, MULT16_16_P15(x2, ADD32(Ksin2, MULT16_16_P15(x2, ADD32(Ksin3, MULT16_16_P15(Ksin4, x2)))))))); /* return cos x as -sin(x - Pi/2) */
		} else { /* x in [3Pi/4, Pi[ -> xScaled in [0, Pi/4], cos(xScaled) = -cos(x) */
			x2 = MULT16_16_P11(xScaled,xScaled); /* in Q0.15 */
			return (SUB32(-Kcos1, MULT16_16_P15(x2, ADD32(Kcos2, MULT16_16_P15(x2, ADD32(Kcos3, MULT16_16_P15(Kcos4, x2))))))); /* return cos x as -cos(Pi -x) */
		}

	}
}
/* KPI6 = pi/6 in Q15 */
#define KPI6 17157 
/* KtanPI6 = tan(pi/6) in Q15 */
#define KtanPI6 18919 
/* KtanPI12 = tan(pi/12) in Q15 */
#define KtanPI12 8780

/* B = 0.257977658811405 in Q15 */
#define atanB 8453
/* C = 0.59120450521312 in Q15 */
#define atanC 19373 

/*****************************************************************************/
/* g729Atan_Q15Q13: ArcTangent fonction in [-2^16, 2^16[                     */
/*    paremeters:                                                            */
/*      -(i) x : 32 bits integer in Q15 in range [-2^16, 2^16[               */
/*    return value:                                                          */
/*      - atan(x) in Q2.13 on 16 bits in range ]-Pi/2(12868), Pi/2(12868)[   */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word16_t g729Atan_Q15Q13(word32_t x)
{
	/* constants for rational polynomial */
	word32_t angle;
	word16_t x2;
	int highSegment = 0;
	int sign = 0;
	int complement = 0;

	/* make argument positive */
	if (x < 0) {
		x = NEG16(x);
		sign = 1;
	}
	
	/* limit argument to 0..1 */
	if(x > ONE_IN_Q15){
		complement = 1;
		x = DIV32(ONE_IN_Q30, x); /* 1/x in Q15 */
	}

	/* determine segmentation */
	if(x > KtanPI12){
		highSegment = 1;
		/* x = (x - k)/(1 + k*x); */
		x = DIV32(SHL(SUB32(x, KtanPI6), 15), ADD32(MULT16_16_Q15(KtanPI6, x), ONE_IN_Q15));
	}

	/* argument is now < tan(15 degrees) */
	/* approximate the function */
	x2 = MULT16_16_Q15(x,x);
	angle = DIV32(MULT16_16(x, ADD32(ONE_IN_Q15, MULT16_16_Q15(atanB, x2))), ADD32(ONE_IN_Q15, MULT16_16_Q15(atanC, x2)));  /* ang = x*(1 + B*x2)/(1 + C*x2) */

	/* now restore offset if needed */
	if(highSegment) {
		angle += KPI6;
	}

	/* restore complement if needed */
	if(complement) {
		angle = SUB32(HALF_PI_Q15, angle);
	}

	/* set result in Q13 */
	angle = PSHR(angle, 2);

	/* restore sign if needed */
	if(sign) {
		return NEG16(angle);
	} else {
		return angle;
	}
}



/*****************************************************************************/
/* g729Asin_Q15Q13: ArcSine fonction                                         */
/*    paremeters:                                                            */
/*      -(i) x : 16 bits integer in Q15 in range ]-1, 1[                     */
/*    return value:                                                          */
/*      - asin(x) in Q2.13 on 16 bits in range ]-Pi/2(12868), Pi/2(12868)[   */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word16_t g729Asin_Q15Q13(word16_t x)
{
	return g729Atan_Q15Q13(DIV32(SHL(x,15), PSHR(g729Sqrt_Q0Q7(SUB32(ONE_IN_Q30, MULT16_16(x,x))),7))); /*  atan(x/sqrt(1.0 - x*x)) */
}

/*****************************************************************************/
/* g729Acos_Q15Q13: ArcCosine fonction                                       */
/*    paremeters:                                                            */
/*      -(i) x : 16 bits integer in Q15 in range ]-1, 1[                     */
/*    return value:                                                          */
/*      - acos(x) in Q2.13 on 16 bits in range ]0, Pi(25736)[                */
/*                                                                           */
/*****************************************************************************/
static BCG729_INLINE word16_t g729Acos_Q15Q13(word16_t x)
{
	return(HALF_PI_Q13 - g729Asin_Q15Q13(x));
}

#endif /* ifndef G729FIXEDPOINTMATH_H */
