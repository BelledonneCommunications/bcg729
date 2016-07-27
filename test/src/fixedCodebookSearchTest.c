/*
 fixedCodebookSearchTest.c

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
/* Test Program for fixedCodebookSearchTest bloc                             */
/*    Input:                                                                 */
/*      -(i) targetSignal: 40 values as in spec A.3.6 in Q0                  */
/*      -(i) impulseResponse: 40 values as in spec A.3.5 in Q12              */
/*      -(i) intPitchDelay: current integer pitch delay                      */
/*      -(i) lastQuantizedAdaptativeCodebookGain: previous subframe pitch    */
/*           gain quantized in Q14                                           */
/*      -(i) filteredAdaptativeCodebookVector : 40 values in Q0              */
/*      -(i) adaptativeCodebookGain : in Q14                                 */
/*                                                                           */
/*    Ouput:                                                                 */
/*      -(o) fixedCodebookParameter                                          */
/*      -(o) fixedCodebookPulsesSigns                                        */
/*      -(o) fixedCodebookVector : 40 values as in spec 3.8, eq45 in Q0      */
/*      -(o) fixedCodebookVectorConvolved : 40 values as in spec 3.9, eq64   */
/*            in Q12                                                         */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "fixedCodebookSearch.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* inputs : */
	word16_t targetSignal[L_SUBFRAME]; /* Q0 */
	word16_t impulseResponse[L_SUBFRAME]; /* Q12 */
	int16_t intPitchDelay;
	word16_t lastQuantizedAdaptativeCodebookGain, adaptativeCodebookGain; /* Q14 */
	word16_t filteredAdaptativeCodebookVector[L_SUBFRAME]; /* Q0 */
	/* outputs :   */
	uint16_t fixedCodebookParameter, fixedCodebookPulsesSigns;
	word16_t fixedCodebookVector[L_SUBFRAME];
	word16_t fixedCodebookVectorConvolved[L_SUBFRAME];

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
		for (i=0; i<40; i++) {
			if (fscanf(fpInput,"%hd,",&(targetSignal[i])) != 1) break;
		}
		for (i=0; i<40; i++) {
			if (fscanf(fpInput,"%hd,",&(impulseResponse[i])) != 1) break;
		}
		if (fscanf(fpInput,"%hd,",&intPitchDelay) != 1) break;
		if (fscanf(fpInput,"%hd,",&lastQuantizedAdaptativeCodebookGain) != 1) break;
		for (i=0; i<40; i++) {
			if (fscanf(fpInput,"%hd,",&(filteredAdaptativeCodebookVector[i])) != 1) break;
		}
		if (fscanf(fpInput,"%hd",&adaptativeCodebookGain) != 1) break;
		
		/* call the tested funtion */
		fixedCodebookSearch(targetSignal, impulseResponse, intPitchDelay, lastQuantizedAdaptativeCodebookGain, filteredAdaptativeCodebookVector, adaptativeCodebookGain,
			&fixedCodebookParameter, &fixedCodebookPulsesSigns, fixedCodebookVector, fixedCodebookVectorConvolved);


		/* write the output to the output file : fixedCodebookParameter, fixedCodebookPulsesSigns, fixedCodebookVector */
		fprintf(fpOutput, "%d,%d,%d", fixedCodebookParameter, fixedCodebookPulsesSigns, fixedCodebookVector[0]);
		for (i=1; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", fixedCodebookVector[i]);
		}
		for (i=0; i<L_SUBFRAME; i++) {
			fprintf(fpOutput,",%d", fixedCodebookVectorConvolved[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

