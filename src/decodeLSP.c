/*
 decodeLSP.c

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
#include "codebooks.h"
#include "utils.h"
#include "g729FixedPointMath.h"

/* define the initialisation vector for qLSP */
/* previous L Code Word initial values (Pi/11 steps) in Q2.13 */
static word16_t previousLCodeWordInit[NB_LSP_COEFF] = {2339, 4679, 7018, 9358, 11698, 14037, 16377, 18717, 21056, 23396};

/* initialisations */
void initDecodeLSP(bcg729DecoderChannelContextStruct *decoderChannelContext)
{
	int i,j;
	/* init the previousLCodeWord buffer according to doc 3.2.4 -> pi/11 steps */
	for (i=0; i<MA_MAX_K; i++) {
		for (j=0; j<NB_LSP_COEFF; j++) {
			decoderChannelContext->previousLCodeWord[i][j] = previousLCodeWordInit[j];
		}
	}	

	/* init the last valid values just to avoid problem in case the first frame is a lost one */
	decoderChannelContext->lastValidL0 = 0;
	for (j=0; j<NB_LSP_COEFF; j++) {
		decoderChannelContext->lastqLSF[j] = previousLCodeWordInit[j];
	}
}


//[MA_MAX_K][NB_LSP_COEFF]; /* in Q2.13, buffer to store the last 4 frames codewords, used to compute the current qLSF */

/*****************************************************************************/
/* computeqLSF : get qLSF extracted from codebooks and process them          */
/*         according to spec 3.2.4                                           */
/*    parameters:                                                            */
/*      -(i/o) codebookqLSF : 10 values i Q2.13 to be updated                */
/*      -(i/o) previousCodeWord : codewords for the last 4 subframes in Q2.13*/
/*                                is updated by this function                */
/*      -(i) L0: the Switched MA predictor retrieved from bitstream          */
/*                                                                           */
/*****************************************************************************/
void computeqLSF(word16_t *codebookqLSF, word16_t previousLCodeWord[MA_MAX_K][NB_LSP_COEFF], uint8_t L0, word16_t currentMAPredictor[L0_RANGE][MA_MAX_K][NB_LSP_COEFF], word16_t currentMAPredictorSum[L0_RANGE][NB_LSP_COEFF]) {
	int i,j;
	word32_t acc; /* Accumulator in Q2.28 */

	/*** rearrange in order to have a minimum distance between two consecutives coefficients ***/
	rearrangeCoefficients(codebookqLSF, GAP1); 
	rearrangeCoefficients(codebookqLSF, GAP2); /* codebookqLSF still in Q2.13 */

	/*** doc 3.2.4 eq(20) ***/
	/* compute the qLSF as a weighted sum(weighted by MA coefficient selected according to L0 value) of previous and current frame L codewords coefficients */

	/* L0 is the Switched MA predictor of LSP quantizer(1 bit) */
	/* codebookqLSF and previousLCodeWord in Q2.13 */
	/* MAPredictor and MAPredictorSum in Q0.15 with MAPredictorSum[MA switch][i]+Sum[j=0-3](MAPredictor[MA switch][j][i])=1 -> acc will end up being in Q2.28*/
	/* Note : previousLCodeWord array containing the last 4 code words is updated during this phase */

	for (i=0; i<NB_LSP_COEFF; i++) { 
		acc = MULT16_16(currentMAPredictorSum[L0][i], codebookqLSF[i]);
		for (j=MA_MAX_K-1; j>=0; j--) {
			acc = MAC16_16(acc, currentMAPredictor[L0][j][i], previousLCodeWord[j][i]); 
			previousLCodeWord[j][i] = (j>0)?previousLCodeWord[j-1][i]:codebookqLSF[i]; /* update the previousqLCodeWord array: row[0] = current code word and row[j]=row[j-1] */
		}
		/* acc in Q2.28, shift back the acc to a Q2.13 with rounding */
		codebookqLSF[i] = (word16_t)PSHR(acc, 15); /* codebookqLSF in Q2.13 */
	}
	/* Note : codebookqLSF buffer now contains qLSF */

	/*** doc 3.2.4 qLSF stability ***/
	/* qLSF in Q2.13 as are qLSF_MIN and qLSF_MAX and MIN_qLSF_DISTANCE */
	
	/* sort the codebookqLSF array */
	insertionSort(codebookqLSF, NB_LSP_COEFF);

	/* check for low limit on qLSF[0] */
	if (codebookqLSF[1]<qLSF_MIN) {
		codebookqLSF[1] = qLSF_MIN;
	}

	/* check and rectify minimum distance between two consecutive qLSF */
	for (i=0; i<NB_LSP_COEFF-1; i++) {
		if (SUB16(codebookqLSF[i+1],codebookqLSF[i])<MIN_qLSF_DISTANCE) {
			codebookqLSF[i+1] = codebookqLSF[i]+MIN_qLSF_DISTANCE;
		}
	}

	/* check for upper limit on qLSF[NB_LSP_COEFF-1] */
	if (codebookqLSF[NB_LSP_COEFF-1]>qLSF_MAX) {
		codebookqLSF[NB_LSP_COEFF-1] = qLSF_MAX;
	}
}

/*****************************************************************************/
/* decodeLSP : decode LSP coefficients as in spec 4.1.1/3.2.4                */
/*    parameters:                                                            */
/*      -(i/o) decoderChannelContext : the channel context data              */
/*      -(i) L: 4 elements array containing L[0-3] the first and             */
/*                     second stage vector of LSP quantizer                  */
/*      -(i) frameErased : a boolean, when true, frame has been erased       */
/*      -(o) qLSP: 10 quantized LSP coefficients in Q15 in range [-1,+1[     */
/*                                                                           */
/*****************************************************************************/
void decodeLSP(bcg729DecoderChannelContextStruct *decoderChannelContext, uint16_t L[], word16_t qLSP[], uint8_t frameErased)
{
	int i,j;
	word16_t currentqLSF[NB_LSP_COEFF]; /* buffer to the current qLSF in Q2.13 */


	if (frameErased == 0) { /* frame is ok, proceed according to 3.2.4 section of the doc */

		/*** doc 3.2.4 eq(19) ***/
		/* get the L codewords from the codebooks L1, L2 and L3 */
		/* for easier implementation, L2 and L3 5 dimensional codebooks have been stored in one 10 dimensional L2L3 codebook */
		/* get the 5 first coefficient from the L1 and L2 codebooks */
		/* Note : currentqLSF buffer contains L codewords and not qLSF */
		for (i=0; i<NB_LSP_COEFF/2; i++) {
			currentqLSF[i] = ADD16(L1[L[1]][i], L2L3[L[2]][i]); /* codebooks are in Q2.13 for L1 and Q0.13 for L2L3, due to actual values stored in the codebooks, result in Q2.13 */
		}

		/* get the 5 last coefficient from L1 and L3 codebooks */
		for ( i=NB_LSP_COEFF/2; i<NB_LSP_COEFF; i++) {
			currentqLSF[i] = ADD16(L1[L[1]][i], L2L3[L[3]][i]); /* same as previous, output in Q2.13 */
		}

		computeqLSF(currentqLSF, decoderChannelContext->previousLCodeWord, L[0], MAPredictor, MAPredictorSum); /* use regular MAPredictor as this function is not called on SID frame decoding */

		/* backup the qLSF and L0 to restore them in case of frame erased */
		for (i=0; i<NB_LSP_COEFF; i++) {
			decoderChannelContext->lastqLSF[i] = currentqLSF[i];
		}
		decoderChannelContext->lastValidL0 = L[0];

	
	} else { /* frame erased indicator is set, proceed according to section 4.4 of the specs */
		word32_t acc; /* acc in Q2.28 */

		/* restore the qLSF of last valid frame */
		for (i=0; i<NB_LSP_COEFF; i++) {
			currentqLSF[i] = decoderChannelContext->lastqLSF[i];
		}
		
		/* compute back the codewords from the qLSF and store them in the previousLCodeWord buffer */
		for (i=0; i<NB_LSP_COEFF; i++) { /* currentqLSF and previousLCodeWord in Q2.13, MAPredictor in Q0.15 and invMAPredictorSum in Q3.12 */
			acc = SHL(decoderChannelContext->lastqLSF[i],15); /* Q2.13 -> Q2.28 */
			for (j=0; j<MA_MAX_K; j++) {
				acc = MSU16_16(acc, MAPredictor[decoderChannelContext->lastValidL0][j][i], decoderChannelContext->previousLCodeWord[j][i]); /* acc in Q2.28 - MAPredictor in Q0.15 * previousLCodeWord in Q2.13 -> acc in Q2.28 (because 1-Sum(MAPred) < 0.6) */
			}
			acc = MULT16_32_Q12(invMAPredictorSum[decoderChannelContext->lastValidL0][i], acc); /* Q3.12*Q2.28 >>12 -> Q2.28 because invMAPredictor is 1/(1 - Sum(MAPred))*/

			/* update the array of previoux Code Words */
			for (j=MA_MAX_K-1; j>=0; j--) {
				/* acc in Q2.28, shift back the acc to a Q2.13 with rounding */
				decoderChannelContext->previousLCodeWord[j][i] = (j>0)?decoderChannelContext->previousLCodeWord[j-1][i]:(word16_t)PSHR(acc, 15); /* update the previousqLSF array: row[0] = computed code word and row[j]=row[j-1] */
			}
		}
	}

	/* convert qLSF to qLSP: qLSP = cos(qLSF) */
	for (i=0; i<NB_LSP_COEFF; i++) {
		qLSP[i] = g729Cos_Q13Q15(currentqLSF[i]); /* ouput in Q0.15 */
	}
	
	/* output: the qLSP buffer in Q0.15 */
	return;
}
