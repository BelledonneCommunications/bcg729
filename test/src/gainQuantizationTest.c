/*
 gainQuantizationTest.c

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
/* Test Program for gainQuantization bloc                                    */
/*    Input:                                                                 */
/*       -(i) targetSignal: 40 values in Q0, x in eq63                       */
/*       -(i) filteredAdaptativeCodebookVector: 40 values in Q0, y in eq63   */
/*       -(i) convolvedFixedCodebookVector: 40 values in Q12, z in eq63      */
/*       -(i) fixedCodebookVector: 40 values in Q13                          */
/*       -(i) xy in Q0 on 64 bits term of eq63 computed previously           */
/*       -(i) yy in Q0 on 64 bits term of eq63 computed previously           */
/*                                                                           */
/*    Ouput:                                                                 */
/*       -(o) quantizedAdaptativeCodebookGain : in Q14                       */
/*       -(o) quantizedFixedCodebookGain : in Q1                             */
/*       -(o) gainCodebookStage1 : GA parameter value (3 bits)               */
/*       -(o) gainCodebookStage2 : GB parameter value (4 bits)               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "gainQuantization.h"

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
	word16_t filteredAdaptativeCodebookVector[L_SUBFRAME]; /* Q0 */
	word16_t convolvedFixedCodebookVector[L_SUBFRAME]; /* Q12 */
	word16_t fixedCodebookVector[L_SUBFRAME]; /* Q13 */
	word64_t xy, yy; /* Q0 */
	/* outputs :   */
	word16_t quantizedAdaptativeCodebookGain, quantizedFixedCodebookGain; /* Q14 */
	uint16_t gainCodebookStage1, gainCodebookStage2;

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
	bcg729EncoderChannelContextStruct *encoderChannelContext = malloc(sizeof(bcg729EncoderChannelContextStruct));

	initGainQuantization(encoderChannelContext);

	/* initialise buffers */

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data until we have some */
		if (fscanf(fpInput,"%lld,%lld",(long long*)&xy,(long long*) &yy) != 2) break;
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(targetSignal[i])) != 1) break;
		}
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(filteredAdaptativeCodebookVector[i])) != 1) break;
		}
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(convolvedFixedCodebookVector[i])) != 1) break;
		}
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(fixedCodebookVector[i])) != 1) break;
		}
		
		
		/* call the tested funtion */
		gainQuantization(encoderChannelContext, targetSignal, filteredAdaptativeCodebookVector, convolvedFixedCodebookVector, fixedCodebookVector, xy, yy,
			&quantizedAdaptativeCodebookGain, &quantizedFixedCodebookGain, &gainCodebookStage1, &gainCodebookStage2);


		/* write the output to the output file : fixedCodebookParameter, fixedCodebookPulsesSigns, fixedCodebookVector */
		fprintf(fpOutput, "%d,%d,%d,%d\n", quantizedAdaptativeCodebookGain, quantizedFixedCodebookGain, gainCodebookStage1, gainCodebookStage2);
	}
	exit (0);
}
