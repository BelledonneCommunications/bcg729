/*
 fixedPointMacros.h

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
#ifndef FIXEDPOINTMACROS_H
#define FIXEDPOINTMACROS_H

#define EXTEND32(x) ((word32_t)(x))
#define NEG16(x) (-(x))
#define NEG32(x) (-(x))

/*** shifts ***/
#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) ((word32_t)(a) << (shift))
#define USHL(a,shift) ((uword32_t)(a) << (shift))
/* shift right with rounding: used to extract the integer value of a Qa number */
#define PSHR(a,shift) (SHR((a)+((EXTEND32(1)<<((shift))>>1)),shift))
/* shift right with checking on sign of shift value */
#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))
#define SHR16(a,shift) ((a) >> (shift))
#define SHL16(a,shift) ((a) << (shift))
#define SHR32(a,shift) ((a) >> (shift))
#define SHL32(a,shift) ((a) << (shift))
#define SHR64(a,shift) ((a) >> (shift))
#define SHL64(a,shift) ((a) << (shift))

/* avoid overflows: a+1 is used to check on negative value because range of a 2n signed bits int is -2pow(n) - 2pow(n)-1 */
/* SATURATE Macro shall be called with MAXINT(nbits). Ex: SATURATE(x,MAXINT16) with MAXINT16  defined to 2pow(16) - 1 */
#define SATURATE(x,a) (((x)>(a) ? (a) : (x)<-(a+1) ? -(a+1) : (x)))

/* absolute value */
#define ABS(a) (((a)>0) ? (a) : -(a))

/*** add and sub ***/
#define ADD16(a,b) ((word16_t)((word16_t)(a)+(word16_t)(b)))
#define SUB16(a,b) ((word16_t)(a)-(word16_t)(b))
#define ADD32(a,b) ((word32_t)(a)+(word32_t)(b))
#define SUB32(a,b) ((word32_t)(a)-(word32_t)(b))

/*** Multiplications/Accumulations ***/
/* WARNING: MULT16_32_QX use MULT16_16 macro but the first multiplication must actually be a 16bits * 32bits with result on 32 bits and not a 16*16 */
/*  MULT16_16 is then implemented here as a 32*32 bits giving result on 32 bits */
#define MULT16_16(a,b)     ((word32_t)((word32_t)(a))*((word32_t)(b)))
#define MULT16_32(a,b)     ((word32_t)((word16_t)(a))*((word32_t)(b)))
#define UMULT16_16(a,b)     ((uword32_t)((word32_t)(a))*((word32_t)(b)))
#define MAC16_16(c,a,b) (ADD32((c),MULT16_16((a),(b))))
#define MSU16_16(c,a,b) (SUB32((c),MULT16_16((a),(b))))
#define DIV32(a,b) (((word32_t)(a))/((word32_t)(b)))
#define UDIV32(a,b) (((uword32_t)(a))/((uword32_t)(b)))

/* Q3 operations */
#define MULT16_16_Q3(a,b) (SHR(MULT16_16((a),(b)),3))
#define MULT16_32_Q3(a,b) ADD32(MULT16_16((a),SHR((b),3)), SHR(MULT16_16((a),((b)&0x00000007)),3))
#define MAC16_16_Q3(c,a,b) ADD32(c,MULT16_16_Q3(a,b))

/* Q4 operations */
#define MULT16_16_Q4(a,b) (SHR(MULT16_16((a),(b)),4))
#define UMULT16_16_Q4(a,b) (SHR(UMULT16_16((a),(b)),4))
#define UMAC16_16_Q4(c,a,b) ADD32(c,UMULT16_16_Q4(a,b))
#define MAC16_16_Q4(c,a,b) ADD32(c,MULT16_16_Q4(a,b))

/* Q11 operations */
#define MULT16_16_Q11(a,b) (SHR(MULT16_16((a),(b)),11))
#define MULT16_16_P11(a,b) (SHR(ADD32(1024,MULT16_16((a),(b))),11))

/* Q12 operations */
#define MULT16_32_Q12(a,b) ADD32(MULT16_16((a),SHR((b),12)), SHR(MULT16_16((a),((b)&0x00000fff)),12))
#define MAC16_32_Q12(c,a,b) ADD32(c,MULT16_32_Q12(a,b))
#define MULT16_16_Q12(a,b) (SHR(MULT16_16((a),(b)),12))
#define MAC16_16_Q12(c,a,b) ADD32(c,MULT16_16_Q12(a,b))
#define MSU16_16_Q12(c,a,b) SUB32(c,MULT16_16_Q12(a,b))

/* Q13 operations */
#define MULT16_16_Q13(a,b) (SHR(MULT16_16((a),(b)),13))
#define MULT16_16_P13(a,b) (SHR(ADD32(4096,MULT16_16((a),(b))),13))
#define MULT16_32_Q13(a,b) ADD32(MULT16_16((a),SHR((b),13)), SHR(MULT16_16((a),((b)&0x00001fff)),13))
#define MAC16_16_Q13(c,a,b) ADD32(c,MULT16_16_Q13(a,b))
#define MAC16_32_Q13(c,a,b) ADD32(c,MULT16_32_Q13(a,b))

/* Q14 operations */
#define MULT16_32_P14(a,b) ADD32(MULT16_16((a),SHR((b),14)), PSHR(MULT16_16((a),((b)&0x00003fff)),14))
#define MULT16_32_Q14(a,b) ADD32(MULT16_16((a),SHR((b),14)), SHR(MULT16_16((a),((b)&0x00003fff)),14))
#define MULT16_16_P14(a,b) (SHR(ADD32(8192,MULT16_16((a),(b))),14))
#define MULT16_16_Q14(a,b) (SHR(MULT16_16((a),(b)),14))
#define MAC16_16_Q14(c,a,b) ADD32(c,MULT16_16_Q14(a,b))
#define MSU16_16_Q14(c,a,b) SUB32(c,MULT16_16_Q14(a,b))
#define MAC16_32_Q14(c,a,b) ADD32(c,MULT16_32_Q14(a,b))

/* Q15 operations */
#define MULT16_16_Q15(a,b) (SHR(MULT16_16((a),(b)),15))
#define MULT16_16_P15(a,b) (SHR(ADD32(16384,MULT16_16((a),(b))),15))
#define MULT16_32_P15(a,b) ADD32(MULT16_16((a),SHR((b),15)), PSHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MULT16_32_Q15(a,b) ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MAC16_32_P15(c,a,b) ADD32(c,MULT16_32_P15(a,b))

/* 64 bits operations */
#define ADD64(a,b) ((word64_t)(a)+(word64_t)(b))
#define SUB64(a,b) ((word64_t)(a)-(word32_t)(b))
#define ADD64_32(a,b) ((word64_t)(a)+(word32_t)(b))
#define MULT32_32(a,b) ((word64_t)((word64_t)(a)*((word64_t)(b))))
#define DIV64(a,b) ((word64_t)(a)/(word64_t)(b))
#define MAC64(c,a,b) ((word64_t)c+(word64_t)((word64_t)a*(word64_t)b))

/* Divisions: input numbers with similar scale(Q) output according to operation. Warning: Make use of 64 bits variables */
#define DIV32_32_Q24(a,b) (((word64_t)(a)<<24)/((word32_t)(b)))
#define DIV32_32_Q27(a,b) (((word64_t)(a)<<27)/((word32_t)(b)))
#define DIV32_32_Q31(a,b) (((word64_t)(a)<<31)/((word32_t)(b)))

#define MULT32_32_Q23(a,b) ((word32_t)(SHR64(((word64_t)a*(word64_t)b),23)))

#define MULT32_32_Q31(a,b) ((word32_t)(SHR64(((word64_t)a*(word64_t)b),31)))
#define MAC32_32_Q31(c,a,b) ADD32(c,MULT32_32_Q31(a,b))

#endif /* ifndef FIXEDPOINTMACROS_H */
