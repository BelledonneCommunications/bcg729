/*
 computeNoiseExcitationTest.c

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
/*    Input: a CSV text with 236 values of 16 bits :                         */
/*           - target gain in Q3                                             */
/*           - pseudo random generator seed                                  */
/*           - 234 values of excitation vector buffer(154 past, 80 current)  */
/*    Ouput: - updated pseudo random generator seed                          */
/*           - 234 16 bits values of updated excitation vector               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "cng.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	word16_t inputBuffer[L_PAST_EXCITATION+L_FRAME]; /* used for input and output, in Q0 */
	word16_t targetGain; /* input in Q3 */
	uint16_t randomGeneratorSeed;

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
		/* read the input data until we have some */
		if (fscanf(fpInput,"%hd",&targetGain) != 1) break;
		if (fscanf(fpInput,",%hd",&randomGeneratorSeed) != 1) break;
		for (i=0; i<L_PAST_EXCITATION+L_FRAME; i++) {
			if (fscanf(fpInput,",%hd",&(inputBuffer[i])) != 1) break;
		}

		/* call the tested function: output will replace the input in the buffer */
		computeComfortNoiseExcitationVector(targetGain, &randomGeneratorSeed, &(inputBuffer[L_PAST_EXCITATION]));

		/* write the output to the output file */
		//fprintf(fpOutput,"%d", (int16_t)randomGeneratorSeed);
		fprintf(fpOutput,"%d", (int16_t)inputBuffer[L_PAST_EXCITATION]);
		for (i=L_PAST_EXCITATION+1; i<L_PAST_EXCITATION+L_FRAME; i++) {
			fprintf(fpOutput,",%d", inputBuffer[i]);
		}
		fprintf(fpOutput,"\n");

	}
	exit (0);
}

