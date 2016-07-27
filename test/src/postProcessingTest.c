/*
 postProcessingTest.c

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
/* Test Program for postProcessing Bloc                                      */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "postProcessing.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* output buffer is the same than input buffer */
	word16_t inputBuffer[L_SUBFRAME]; /* 40 values in Q0 */

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
	/* create the context structure */
	bcg729DecoderChannelContextStruct *decoderChannelContext = malloc(sizeof(bcg729DecoderChannelContextStruct));

	initPostProcessing(decoderChannelContext);

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data until we have some */
		if (fscanf(fpInput,"%hd",&(inputBuffer[0])) != 1) break;
		for (i=1; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(inputBuffer[i])) != 1) break;
		}

		/* call the preProcessing function: output will replace the input in the buffer */
		postProcessing(decoderChannelContext, inputBuffer);

		/* write the output to the output file */
		fprintf(fpOutput,"%d",inputBuffer[0]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d",inputBuffer[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

