/*
 codebooks.h

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
#ifndef CODEBOOKS_H
#define CODEBOOKS_H

#include "codecParameters.h"
/* this codebooks are defined in codebook.c */

/*** codebooks for quantization of the LSP coefficient - doc: 3.2.4 ***/
extern word16_t L1[L1_RANGE][NB_LSP_COEFF]; /* The first stage is a 10-dimensional VQ using codebook L1 with 128 entries (7 bits). in Q2.13 */
extern word16_t L2L3[L2_RANGE][NB_LSP_COEFF]; /* Doc : The second stage is a 10-bit VQ splitted in L2(first 5 values of a vector) and L3(last five value in each vector) containing 32 entries (5 bits). in Q0.13 but max value < 0.5 so fits in 13 bits. */

extern word16_t MAPredictor[L0_RANGE][MA_MAX_K][NB_LSP_COEFF]; /* the MA predictor coefficients in Q0.15 but max value < 0.5 so it fits on 15 bits */
extern word16_t MAPredictorSum[L0_RANGE][NB_LSP_COEFF]; /* 1 - Sum(MAPredictor) in Q0.15 */
extern word16_t invMAPredictorSum[L0_RANGE][NB_LSP_COEFF]; /* 1/(1 - Sum(MAPredictor)) in Q3.12 */
/* same as above but used for SID frame */
extern word16_t noiseMAPredictor[L0_RANGE][MA_MAX_K][NB_LSP_COEFF]; /* the MA predictor coefficients in Q0.15 but max value < 0.5 so it fits on 15 bits */
extern word16_t noiseMAPredictorSum[L0_RANGE][NB_LSP_COEFF]; /* 1 - Sum(MAPredictor) in Q0.15 */
extern word16_t invNoiseMAPredictorSum[L0_RANGE][NB_LSP_COEFF]; /* 1/(1 - Sum(MAPredictor)) in Q3.12 */

/* codebook for adaptative code vector */
extern word16_t b30[31];

/* codebook for gains */
extern uint16_t reverseIndexMappingGA[8];
extern uint16_t reverseIndexMappingGB[16];
extern uint16_t indexMappingGA[8];
extern uint16_t indexMappingGB[16];
extern word16_t GACodebook[8][2];
extern word16_t GBCodebook[16][2];
extern word16_t MAPredictionCoefficients[4];
extern word16_t SIDGainCodebook[32];
extern uint8_t L1SubsetIndex[32];
extern uint8_t L2SubsetIndex[16];
extern uint8_t L3SubsetIndex[16];

/* codebook for VAD */
extern word16_t lowBandFilter[NB_LSP_COEFF+3];

/* codebook for LP Analysis */
extern word16_t wlp[L_LP_ANALYSIS_WINDOW];
extern word16_t wlag[NB_LSP_COEFF+3];
#endif /* ifndef CODEBOOKS_H */
