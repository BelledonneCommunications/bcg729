/*
 * Copyright (c) 2011-2019 Belledonne Communications SARL.
 *
 * This file is part of bcg729.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef FINDOPENLOOPPITCHDELAY_H
#define FINDOPENLOOPPITCHDELAY_H
/*****************************************************************************/
/* findOpenLoopPitchDelay : as specified in specA3.4                         */
/*    paremeters:                                                            */
/*      -(i) weightedInputSignal: 223 values in Q0, buffer                   */
/*           accessed in range [-MAXIMUM_INT_PITCH_DELAY(143), L_FRAME(80)[  */
/*    return value:                                                          */
/*      - the openLoopIntegerPitchDelay in Q0 range [20, 143]                */
/*                                                                           */
/*****************************************************************************/
uint16_t findOpenLoopPitchDelay(word16_t weightedInputSignal[]);
#endif /* ifndef FINDOPENLOOPPITCHDELAY_H */
