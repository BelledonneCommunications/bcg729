/*
 LSPQuantizationTest.c

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
/* Test Program for LSPQuantization bloc                                     */
/*    Input: - 10 LSP in Q15                                                 */
/*                                                                           */
/*    Ouput: - 10 qLSP in Q15                                                */
/*           - 4 parameters L0, L1, L2, L3                                   */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "LSPQuantization.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: 10 LSP Coefficients(Q15) */
	word16_t LSP[NB_LSP_COEFF];
	
	/* output file: 10 qLSP Coefficients(Q15) */
	word16_t qLSP[NB_LSP_COEFF];
	uint16_t parametersL[4];


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

	initLSPQuantization(encoderChannelContext);

	/* initialise buffers */

	/*** initialisation complete ***/
//		LSPQuantization(LSP, qLSP, parametersL);
//exit (0);

	/*** loop over input file ***/
	while(fscanf(fpInput,"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",&LSP[0],&LSP[1],&LSP[2],&LSP[3],&LSP[4],&LSP[5],&LSP[6],&LSP[7],&LSP[8],&LSP[9]) == 10) 
	{
		int i;
		
		/* call the tested funtion */
		LSPQuantization(encoderChannelContext, LSP, qLSP, parametersL);
 
		/* write the output to the output file */
		fprintf(fpOutput,"%d", qLSP[0]);
		for (i=1; i<NB_LSP_COEFF; i++) {
			fprintf(fpOutput,",%d", qLSP[i]);
		}
		fprintf(fpOutput,",%d,%d,%d,%d\n",parametersL[0],parametersL[1],parametersL[2],parametersL[3]);
	}
	exit (0);
}

