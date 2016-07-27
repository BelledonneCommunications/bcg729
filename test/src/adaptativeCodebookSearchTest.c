/*
 adaptativeCodebookSearchTest.c

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
/* Test Program for adaptativeCodebookSearchTest bloc                        */
/*    Input: - the past excitation (excitationVector) : 234 values           */
/*           - targetSignal : 40 values                                      */
/*           - impulseResponse : 40 values                                   */
/*           - intPitchDelayMin                                              */
/*           - intPitchDelayMax                                              */
/*           - subframeIndex (0 or 40)                                       */
/*                                                                           */
/*    Ouput: - int Pitch Delay                                               */
/*           - frac Pitch Delay                                              */
/*           - intPitchDelayMin                                              */
/*           - intPitchDelayMax                                              */
/*           - adaptativeCodebookIndex                                       */
/*           - adaptative codebook vector : 40 values                        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "adaptativeCodebookSearch.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: pastExcitationVector: 234 values, targetSignal: 40 values, impulseReponse: 40 values, intPitchDelayMin, intPitchDelayMax, subframeIndex */
	word16_t excitationVector[L_PAST_EXCITATION + L_FRAME]; /* this vector contains: note on second subframe we have 40 useless values of past excitation
						0->153 : the past excitation vector.(length is Max Pitch Delay: 144 + interpolation window size : 10)
						154-> 154+L_SUBFRAME-1 : the current LPResidualSignal in input
						154-> 154+L_SUBFRAME-1 : the adaptative codebook vector in Q0 as output */
	word16_t targetSignal[L_SUBFRAME];
	word16_t impulseResponse[L_SUBFRAME];
	int16_t intPitchDelayMin, intPitchDelayMax;
	int subFrameIndex;
	/* outputs :   */
	int16_t intPitchDelay; 
	int16_t fracPitchDelay; 
	uint16_t adaptativeCodebookIndex;

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

	/* initialise buffers */
	/* actually useless init, because the complete excitation buffer is reload at each subframe from input file, but shall be done in real decoder */
	memset(excitationVector, 0, L_PAST_EXCITATION*sizeof(word16_t)); /* initialise the part of the excitationVector containing the past excitation */
	

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data until we have some */
		for (i=0; i<234; i++) {
			if (fscanf(fpInput,"%hd,",&(excitationVector[i])) != 1) break;
		}
		for (i=0; i<40; i++) {
			if (fscanf(fpInput,"%hd,",&(targetSignal[i])) != 1) break;
		}
		for (i=0; i<40; i++) {
			if (fscanf(fpInput,"%hd,",&(impulseResponse[i])) != 1) break;
		}
		if (fscanf(fpInput,"%hd,",&intPitchDelayMin) != 1) break;
		if (fscanf(fpInput,"%hd,",&intPitchDelayMax) != 1) break;
		if (fscanf(fpInput,"%d",&subFrameIndex) != 1) break;
		
		/* call the tested funtion */
		adaptativeCodebookSearch(&(excitationVector[L_PAST_EXCITATION + subFrameIndex]), &intPitchDelayMin, &intPitchDelayMax, impulseResponse, targetSignal,
				&intPitchDelay, &fracPitchDelay, &adaptativeCodebookIndex, subFrameIndex);

		/* write the output to the output file : intPitchDelay, fracPitchDelay, intPitchDelayMin, intPitchDelayMax, adaptativeCodebookIndex, adaptative codebook vector */
		fprintf(fpOutput, "%d,%d,%d,%d,%d,%d", intPitchDelay, fracPitchDelay, intPitchDelayMin, intPitchDelayMax, adaptativeCodebookIndex, excitationVector[L_PAST_EXCITATION+subFrameIndex]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", excitationVector[L_PAST_EXCITATION+subFrameIndex+i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

