/*
 decodeFixedCodeVectorTest.c

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
/* Test Program for decodeFixedCodeVectorTest bloc                           */
/*    Input: - signs: parameter S(4 signs bit) eq61                          */
/*           - positions: parameter C(4 3bits position and jx bit) eq62      */
/*           - intPitchDelay: integer part of pitch Delay (T)                */
/*           - boundedPitchGain: Beta in eq47 and eq48, in Q14               */
/*                                                                           */
/*    Ouput: - fixedCodebookVector for current subframe in Q13               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "decodeFixedCodeVector.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: Positions(C parameter 13 bits), Signs(S parameter 4 bits), boundedPitchGain, intPitchDelay  */
	/* output: fixedCodebookVector: subframe length vector in Q13 */
	int16_t signs;
	int16_t positions;
	word16_t boundedPitchGain;
	int16_t intPitchDelay;

	word16_t fixedCodebookVector[L_SUBFRAME];

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
	while(fscanf(fpInput,"%hd,%hd,%hd,%hd", &positions, &signs, &boundedPitchGain, &intPitchDelay) == 4) /* loop until end of input data */
	{
		int i;
		
		/* call the tested funtion */
		decodeFixedCodeVector((uint16_t)signs, (uint16_t)positions, (int16_t)intPitchDelay, (word16_t)boundedPitchGain, fixedCodebookVector);


		/* write the output to the output file */
		fprintf(fpOutput,"%d",fixedCodebookVector[0]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", fixedCodebookVector[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

