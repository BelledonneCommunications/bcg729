/*
 computeLPTest.c

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
/* Test Program for computeLP Bloc                                           */
/*    Input: a CSV text with 240 values of 16 bits PCM on each row           */
/*    Ouput: 10 LP values in Q12                                             */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"
#include "basicOperationsMacros.h"

#include "testUtils.h"

#include "computeLP.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	word16_t inputBuffer[L_LP_ANALYSIS_WINDOW]; 
	word16_t LPCoefficients[NB_LSP_COEFF];

	/*** inits ***/
	/* open the input file */
	if ( (fpInput = fopen(argv[1], "r")) == NULL) {
		printf("%s - Error: can't open file  %s\n", argv[0], argv[1]);
		exit(-1);
	}

	/* create the output file(filename is the same than input file with the .out extension) */
	char *outputFile = malloc((strlen(filePrefix)+5)*sizeof(char));
	sprintf(outputFile, "%s.out",filePrefix);
	if ( (fpOutput = fopen(outputFile, "w")) == NULL) {
		printf("%s - Error: can't create file  %s\n", argv[0], outputFile);
		exit(-1);
	}
	
	/*** init of the tested bloc ***/

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* by-products of computeLP used for VAD/DTX, not tested here */
		word32_t reflectionCoefficients[10];
		word32_t autoCorrelationCoefficients[11];
		word32_t noLagAutoCorrelationCoefficients[11];
		int8_t autoCorrelationCoefficientsScale;

		/* read the input data until we have some */
		if (fscanf(fpInput,"%hd",&(inputBuffer[0])) != 1) break;
		for (i=1; i<L_LP_ANALYSIS_WINDOW; i++) {
			if (fscanf(fpInput,",%hd",&(inputBuffer[i])) != 1) break;
		}

		/* call the preProcessing function: output will replace the input in the buffer */
		computeLP(inputBuffer, LPCoefficients, reflectionCoefficients,  autoCorrelationCoefficients, noLagAutoCorrelationCoefficients, &autoCorrelationCoefficientsScale, 11);

		/* write the output to the output file */
		fprintf(fpOutput,"%d", LPCoefficients[0]);
		for (i=1; i<NB_LSP_COEFF; i++) {
			fprintf(fpOutput,",%d", LPCoefficients[i]);
		}
		fprintf(fpOutput,"\n");

	}
	exit (0);
}

