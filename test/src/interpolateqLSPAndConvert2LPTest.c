/*
 interpolateqLSPAndConvert2LPTest.c

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
/* Test Program for interpolateqLSP and qLSP2LP Blocs                        */
/*    Input: 10 qLSP coefficients in word16_t format in Q0.15                */
/*           a text CSV file, 10 values on each row.                         */
/*    Ouput: the 10 LP coefficients for first and second subframe            */
/*           20 elements vectors in a text CSV file                          */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "interpolateqLSP.h"
#include "qLSP2LP.h"

/* buffers allocation */
static word16_t previousqLSP[NB_LSP_COEFF];
static const word16_t previousqLSPInitialValues[NB_LSP_COEFF] = {30000, 26000, 21000, 15000, 8000, 0, -8000,-15000,-21000,-26000}; /* in Q0.15 the initials values for the previous qLSP buffer */

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	word16_t inputBuffer[NB_LSP_COEFF]; /* input buffer: an array containing the 10 qLSP coefficients for the current frame */
	word16_t outputBuffer[2*NB_LSP_COEFF]; /* output buffer: the LP coefficients in Q2.13 for subframe 1 and 2 (20 LP coefficients)*/ 

	word16_t interpolatedqLSP[NB_LSP_COEFF]; /* the interpolated qLSP used to produce 1st subframe LP */

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
	memcpy(previousqLSP, previousqLSPInitialValues, NB_LSP_COEFF*sizeof(word16_t)); /* initialise the previousqLSP buffer */

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(fscanf(fpInput, "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd", &(inputBuffer[0]), &(inputBuffer[1]), &(inputBuffer[2]), &(inputBuffer[3]), &(inputBuffer[4]), &(inputBuffer[5]), &(inputBuffer[6]), &(inputBuffer[7]), &(inputBuffer[8]), &(inputBuffer[9]))==10)
	{ /* input buffer contains current qLSP */

		/* call the interpolate function, result will be stored in the interpolatedqLSP buffer */
		interpolateqLSP(previousqLSP, inputBuffer, interpolatedqLSP);

		int i;
		/* copy the currentqLSP to previousqLSP buffer */
		for (i=0; i<NB_LSP_COEFF; i++) {
			previousqLSP[i] = inputBuffer[i];
		}

		/* call the qLSP2LP function for first subframe */
		qLSP2LP(interpolatedqLSP, outputBuffer);

		/* call the qLSP2LP function for second subframe */
		qLSP2LP(inputBuffer, &(outputBuffer[NB_LSP_COEFF]));

		/* write the output to the output file */
		fprintf(fpOutput,"%d",outputBuffer[0]);
		for (i=1; i<2*NB_LSP_COEFF; i++) {
			fprintf(fpOutput,",%d",outputBuffer[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

