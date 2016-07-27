/*
 decodeAdaptativeCodeVectorTest.c

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
/* Test Program for decodeAdaptativeCodeVectorTest bloc                      */
/*    Input: - the past excitation (excitationVector)                        */
/*           - subframeIndex                                                 */
/*           - frameErasureFlag                                              */
/*           - parityFlag                                                    */
/*           - adaptative Codebook Index (P1 or P2)                          */
/*                                                                           */
/*    Ouput: - adaptative Codebook Vector for the current subframe           */
/*           - int Pitch Delay (T)                                           */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "decodeAdaptativeCodeVector.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: frameErasureFlag, subframeIndex, P1/2, P0, pastExcitationVector: 154 values */
	/* output is in the excitationVector */
	int16_t frameErasureFlag;
	int subFrameIndex;
	uint16_t adaptativeCodebookIndex;
	int16_t parityFlag;
	word16_t excitationVector[L_PAST_EXCITATION + L_FRAME]; /* this vector contains: 
						0->153 : the past excitation vector.(length is Max Pitch Delay: 144 + interpolation window size : 10)
						154-> 154+L_FRAME-1 : the current frame adaptative Code Vector first used to compute then the excitation vector */
	int16_t intPitchDelay; /* output of the function when called for subframe wich is sent back as an input for subframe2 */

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

	initDecodeAdaptativeCodeVector(decoderChannelContext);

	/* initialise buffers */
	/* actually useless init, because the complete excitation buffer is reload at each subframe from input file, but shall be done in real decoder */
	memset(excitationVector, 0, L_PAST_EXCITATION*sizeof(word16_t)); /* initialise the part of the excitationVector containing the past excitation */
	

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data until we have some */
		if (fscanf(fpInput,"%hd",&frameErasureFlag) != 1) break;
		if (fscanf(fpInput,",%d",&subFrameIndex) != 1) break;
		if (fscanf(fpInput,",%hd",&adaptativeCodebookIndex) != 1) break;
		if (fscanf(fpInput,",%hd",&parityFlag) != 1) break;
		for (i=0; i<194; i++) {
			if (fscanf(fpInput,",%hd",&(excitationVector[i])) != 1) break;
		}
		
		/* call the tested funtion */
		decodeAdaptativeCodeVector(decoderChannelContext, subFrameIndex, adaptativeCodebookIndex, (uint8_t)parityFlag, (uint8_t)frameErasureFlag,
				&intPitchDelay, &(excitationVector[L_PAST_EXCITATION + subFrameIndex]));


		/* write the output to the output file */
		fprintf(fpOutput,"%d",excitationVector[L_PAST_EXCITATION + subFrameIndex]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", excitationVector[L_PAST_EXCITATION + subFrameIndex+i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

