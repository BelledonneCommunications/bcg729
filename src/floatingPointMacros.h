/*
 floatingPointMacros.h

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
#ifndef FLOATINGPOINTMACROS_H
#define FLOATINGPOINTMACROS_H

#define EXTEND32(x) (x)

/* shifts */
#define SHR(a,shift) (a)
#define SHL(a,shift) (a)
#define PSHR(a,shift) (a)

/* avoid overflows: nothing to do for floats */
#define SATURATE(x,a) (x)

/* add and sub */
#define ADD16(a,b) ((a)+(b))
#define SUB16(a,b) ((a)-(b))
#define ADD32(a,b) ((a)+(b))
#define SUB32(a,b) ((a)-(b))


/* Multiplications/Accumulations */
#define MULT16_16(a,b)     ((word32_t)(a)*(word32_t)(b))
#define MAC16_16(c,a,b)    ((c)+MULT16_16((a),(b)))
#define MULT16_32(a,b)     ((a)*(b))
#define MAC16_32(c,a,b)     ((c)+(a)*(b))

/* Q12 operations */
#define MULT16_32_Q12(a,b)	MULT16_32(a,b)
#define MAC16_32_Q12(c,a,b)	MAC16_32(c,a,b)


#endif /* ifndef FLOATINGPOINTMACROS_H */
