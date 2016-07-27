/*
 postFilterTest.c

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
/* Test Program for postFilter                                               */
/*    Input: - LPCoefficients: 10 LP coeff for current subframe(in Q12)      */
/*           - reconstructedSpeech: output of LP Synthesis                   */
/*             50 values in Q0: need 10 values of previous subframe          */
/*             buffer is accessed in range [-10, 39]                         */
/*           - intPitchDelay: the integer part of Pitch Delay(in Q0)         */
/*           - subframeIndex: 0 or L_SUBFRAME for subframe 0 or 1            */
/*           - residualSignal: 143+40 values of previous frames computation  */
/*                                                                           */
/*    Ouput: - long term postfilered signal (40 values in Q0)                */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "postFilter.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: 10 LP Coefficients(Q12), 50 reconstructedSpeech, intPitchDelay (Q0), subframeIndex(0/40) */
	word16_t LP[NB_LSP_COEFF];
	word16_t reconstructedSpeech[NB_LSP_COEFF+L_SUBFRAME]; /* in Q0, output of the LP synthesis filter, the first 10 words store the previous subframe output */
	int16_t intPitchDelay;
	int16_t subframeIndex;
	
	/* output postFiltered signal(40 values in Q0) */
	word16_t postFilteredSignal[L_SUBFRAME];

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

	initPostFilter(decoderChannelContext);

	/* initialise buffers */
	/* past reconstructedSpeech values shall be initialised, but we read them directly from input file */

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data as long as we find some */
		if (fscanf(fpInput,"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",&LP[0],&LP[1],&LP[2],&LP[3],&LP[4],&LP[5],&LP[6],&LP[7],&LP[8],&LP[9]) != 10) break;
		for (i=0; i<L_SUBFRAME+NB_LSP_COEFF; i++) {
			if (fscanf(fpInput,",%hd",&(reconstructedSpeech[i])) != 1) break;
		}
		if (fscanf(fpInput,",%hd",&intPitchDelay) != 1) break;
		if (fscanf(fpInput,",%hd",&subframeIndex) != 1) break;
		
		/* call the tested funtion */
		postFilter(decoderChannelContext, LP, &(reconstructedSpeech[NB_LSP_COEFF]), intPitchDelay, subframeIndex, postFilteredSignal);

		/* write the output to the output file */
		fprintf(fpOutput,"%d", postFilteredSignal[0]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", postFilteredSignal[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

