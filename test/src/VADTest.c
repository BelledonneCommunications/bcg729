/*
 VADTest.c

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
/*    Input: a CSV text with 107 values :                                    */
/*           - reflectionCoefficient in Q31                                  */
/*           - 10 LSF in Q13                                                 */
/*           - autocorrelation exponent                                      */
/*           - 13 autocorrelation coefficients                               */
/*           - 82 values of input signal                                     */
/*    Ouput: - VAD : 0 or 1                                                  */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "vad.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	word32_t reflectionCoefficient; /* in Q31 */
	word16_t LSFCoefficients[NB_LSP_COEFF];
	int16_t autocorrelationExponent;
	word32_t autocorrelationCoefficients[NB_LSP_COEFF+3];
	word16_t signalBuffer[82]; /* used for input and output, in Q0 */

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
	bcg729VADChannelContextStruct *VADChannelContext = initBcg729VADChannel();

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		uint8_t vad;

		/* read the input data until we have some */
		if (fscanf(fpInput,"%d",&reflectionCoefficient) != 1) break;
		for (i=0; i<10; i++) {
			if (fscanf(fpInput,",%hd",(int16_t *)&(LSFCoefficients[i])) != 1) break;
		}
		if (fscanf(fpInput,",%hd",&autocorrelationExponent) != 1) break;
		for (i=0; i<13; i++) {
			if (fscanf(fpInput,",%d",&(autocorrelationCoefficients[i])) != 1) break;
		}
		for (i=0; i<82; i++) {
			if (fscanf(fpInput,",%hd",(int16_t *)&(signalBuffer[i])) != 1) break;
		}

		/* call the tested function: output will replace the input in the buffer */
		vad = bcg729_vad(VADChannelContext, reflectionCoefficient, LSFCoefficients, autocorrelationCoefficients, autocorrelationExponent, &(signalBuffer[1]));

		/* write the output to the output file */
		//fprintf(fpOutput,"%d", (int16_t)randomGeneratorSeed);
		fprintf(fpOutput,"%d\n", vad);

	}
	exit (0);
}

