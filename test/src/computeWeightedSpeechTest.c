/*
 computeWeightedSpeechTest.c

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
/* Test Program for computeWeightedSpeech bloc                               */
/*    Input: 90 values input signal, 10 values of previous weighted signal   */
/*           20 values of qLPCoefficients, 20 values of weightedqLPCoeffs    */
/*    Ouput: 80 values of weightedSignal                                     */
/*           80 values of LP Residual Signal                                 */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "computeWeightedSpeech.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	word16_t inputBuffer[NB_LSP_COEFF+L_FRAME]; /* buffer containing the input signal including 10 values from last frame in range [-10, 0[ */
	word16_t outputBuffer[NB_LSP_COEFF+L_FRAME]; /* same as input buffer, the values in range [-10,0[ are input */
	word16_t qLPCoefficients[2*NB_LSP_COEFF]; /* qLP coefficients in Q12: input */
	word16_t weightedqLPCoefficients[2*NB_LSP_COEFF]; /* weightedqLP coefficients in Q12: input */
	word16_t LPResidualSignal[L_FRAME]; /* LP Residual signal in Q0: ouput*/

	/*** inits ***/
	/* open the input file */
	if ( (fpInput = fopen(argv[1], "rb")) == NULL) {
		printf("%s - Error: can't open file  %s\n", argv[0], argv[1]);
		exit(-1);
	}

	/* create the output file(filename is the same than input file with the .out extension) */
	char *outputFile = malloc((strlen(filePrefix)+5)*sizeof(char));
	sprintf(outputFile, "%s.out",filePrefix);
	if ( (fpOutput = fopen(outputFile, "wb")) == NULL) {
		printf("%s - Error: can't create file  %s\n", argv[0], outputFile);
		exit(-1);
	}
	
	/*** init of the tested bloc ***/

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/*** read the input data until we have some ***/
		/* read the input signal */
		if (fscanf(fpInput,"%hd",&(inputBuffer[0])) != 1) break;
		for (i=1; i<L_FRAME+NB_LSP_COEFF; i++) {
			if (fscanf(fpInput,",%hd",&(inputBuffer[i])) != 1) break;
		}
		/* read 10 values of weighted signal from previous frame */
		for (i=0; i<NB_LSP_COEFF; i++) {
			if (fscanf(fpInput,",%hd",&(outputBuffer[i])) != 1) break; /* the first 10 values of output buffer are input: weighted signal from previous frame */
		}
		/* read qLPCoefficients */
		for (i=0; i<2*NB_LSP_COEFF; i++) {
			if (fscanf(fpInput,",%hd",&(qLPCoefficients[i])) != 1) break; 
		}
		/* read weightedqLPCoefficients */
		for (i=0; i<2*NB_LSP_COEFF; i++) {
			if (fscanf(fpInput,",%hd",&(weightedqLPCoefficients[i])) != 1) break; 
		}

		/* call the computeWeightedSignal function */
		computeWeightedSpeech(&(inputBuffer[NB_LSP_COEFF]), qLPCoefficients, weightedqLPCoefficients, &(outputBuffer[NB_LSP_COEFF]), LPResidualSignal); /* input and output buffer shall be accessed by the tested function in range [-NB_LSP_COEFF, LFRAME[ */

		/* write the output to the output file */
		fprintf(fpOutput,"%d", outputBuffer[NB_LSP_COEFF]);
		for (i=1; i<L_FRAME; i++) {
			fprintf(fpOutput,",%d", outputBuffer[NB_LSP_COEFF+i]);
		}
		for (i=0; i<L_FRAME; i++) {
			fprintf(fpOutput,",%d", LPResidualSignal[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

