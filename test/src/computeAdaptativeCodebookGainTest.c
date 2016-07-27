/*
 computeAdaptativeCodebookGainTest.c

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
/* Test Program for computeAdaptativeCodebookGainTest bloc                   */
/*    Input: - targetSignal : 40 values                                      */
/*           - filtered adaptative codebook vector : 40 values               */
/*                                                                           */
/*    Ouput: - adaptative codebook gain in Q14                               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "computeAdaptativeCodebookGain.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	word16_t targetSignal[L_SUBFRAME];
	word16_t filteredAdaptativeCodebookVector[L_SUBFRAME];
	/* outputs :   */
	word16_t adaptativeCodebookGain;

	/* these buffers are part of the output but not of the pattern as they aren't scaled the same way the ITU code does. */
	word64_t gainQuantizationXy, gainQuantizationYy; /* used to store in Q0 values reused in gain quantization */

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
		if (fscanf(fpInput,"%hd",&(filteredAdaptativeCodebookVector[0])) != 1) break;
		for (i=1; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(filteredAdaptativeCodebookVector[i])) != 1) break;
		}
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(targetSignal[i])) != 1) break;
		}
		
		/* call the tested funtion */
		adaptativeCodebookGain = computeAdaptativeCodebookGain(targetSignal, filteredAdaptativeCodebookVector, &gainQuantizationXy, &gainQuantizationYy);

		/* write the output to the output file : adaptativeCodebookGain */
		fprintf(fpOutput,"%d\n", adaptativeCodebookGain);
	}
	exit (0);
}

