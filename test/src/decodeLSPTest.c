/*
 decodeLSPTest.c

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
/* Test Program for decodeLSP bloc                                           */
/*    Input: L coefficient files: L0, L1, L2 and L3 in word16_t format in    */
/*           a text CSV file, 4 values on each row.                          */
/*    Ouput: the output of the decodeLSP bloc in a file: series of word16_t  */
/*           10 elements vectors in a text CSV file                          */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "decodeLSP.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	uint16_t inputBuffer[4]; /* input buffer: an array containing the 4 L coefficients */
	word16_t frameErasedIndicator; /* part of the input file */
	word16_t outputBuffer[NB_LSP_COEFF]; /* output buffer: the quantized LSF coefficients in Q0.15 */ 

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
	/* create the context structure */
	bcg729DecoderChannelContextStruct *decoderChannelContext = malloc(sizeof(bcg729DecoderChannelContextStruct));
	initDecodeLSP(decoderChannelContext);

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(fscanf(fpInput, "%hd,%hd,%hd,%hd,%hd", &frameErasedIndicator, &(inputBuffer[0]), &(inputBuffer[1]), &(inputBuffer[2]), &(inputBuffer[3]))==5)
	{
		/* call the decodeLSP function */
		decodeLSP(decoderChannelContext, inputBuffer, outputBuffer, frameErasedIndicator);

		/* write the output to the output file */
		fprintf(fpOutput,"%d",outputBuffer[0]);
		int i;
		for (i=1; i<NB_LSP_COEFF; i++) {
			fprintf(fpOutput,",%d",outputBuffer[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

