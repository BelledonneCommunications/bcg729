/*
 LPSynthesisFilterTest.c

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
/* Test Program for LPSynthesisFilter bloc                                   */
/*    Input: -                                                               */
/*                                                                           */
/*    Ouput: -                                                               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "LPSynthesisFilter.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: 10 LP Coefficients(Q12), 40 ExcitationVector(Q0), 10 past reconstructed speech(Q0) */
	word16_t LP[NB_LSP_COEFF];
	word16_t excitationVector[L_SUBFRAME]; /* this vector contains the subframe excitation vector in Q0 */
	
	/* output is in the reconstruted speech: for the test purpose it's long just memory+subframe  */
	word16_t reconstructedSpeech[NB_LSP_COEFF+L_SUBFRAME]; /* in Q0, output of the LP synthesis filter, the first 10 words store the previous frame output */

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

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data until we have some */
		if (fscanf(fpInput,"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",&LP[0],&LP[1],&LP[2],&LP[3],&LP[4],&LP[5],&LP[6],&LP[7],&LP[8],&LP[9]) != 10) break;
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(excitationVector[i])) != 1) break;
		}
		if (fscanf(fpInput,",%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",&reconstructedSpeech[0],&reconstructedSpeech[1],&reconstructedSpeech[2],&reconstructedSpeech[3],&reconstructedSpeech[4],&reconstructedSpeech[5],&reconstructedSpeech[6],&reconstructedSpeech[7],&reconstructedSpeech[8],&reconstructedSpeech[9]) != 10) break;
		
		/* call the tested funtion */
		LPSynthesisFilter(excitationVector, LP, &(reconstructedSpeech[NB_LSP_COEFF]));

		/* write the output to the output file */
		fprintf(fpOutput,"%d",reconstructedSpeech[NB_LSP_COEFF]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", reconstructedSpeech[NB_LSP_COEFF+i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

