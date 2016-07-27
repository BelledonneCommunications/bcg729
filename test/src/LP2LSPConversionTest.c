/*
 LP2LSPConversionTest.c

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
/* Test Program for LP2LSPConversion bloc                                    */
/*    Input: - 10 LP in Q12                                                  */
/*                                                                           */
/*    Ouput: - 10 LSP in Q15                                                 */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "LP2LSPConversion.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: 10 LP Coefficients(Q12) */
	word16_t LP[NB_LSP_COEFF];
	
	/* output file: 10 LSP Coefficients(Q15) */
	word16_t LSP[NB_LSP_COEFF];
	word16_t previousLSP[NB_LSP_COEFF] = /* store previously computed LSP to be reused if we can't find 10 LSP from current LP */
 	{30000, 26000, 21000, 15000, 8000, 0, -8000,-15000,-21000,-26000};


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
	while(fscanf(fpInput,"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",&LP[0],&LP[1],&LP[2],&LP[3],&LP[4],&LP[5],&LP[6],&LP[7],&LP[8],&LP[9]) == 10) 
	{
		int i;
		
		/* call the tested funtion */
		if (LP2LSPConversion(LP, LSP)) {
			for (i=0; i<NB_LSP_COEFF; i++) {
				previousLSP[i]=LSP[i];
			}
		} else { /* unable to find the 10 roots repeat previous LSP */
			for (i=0; i<NB_LSP_COEFF; i++) {
				LSP[i]=previousLSP[i];
			}
		}
		/* write the output to the output file */
		fprintf(fpOutput,"%d", LSP[0]);
		for (i=1; i<NB_LSP_COEFF; i++) {
			fprintf(fpOutput,",%d", LSP[i]);
		}
		fprintf(fpOutput,"\n");
	}
	exit (0);
}

