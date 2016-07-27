/*
 decodeGainsTest.c

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
/* Test Program for decodeGains bloc                                         */
/*    Input: - GA parameter                                                  */
/*           - GB parameter                                                  */
/*           - frameErasureFlag                                              */
/*           - adaptative Codebook Gain from previous frame                  */
/*           - fixed Codebook Gain from previous frame                       */
/*           - fixed Codebook Vector                                         */
/*                                                                           */
/*    Ouput: - adaptative Codebook Gain                                      */
/*           - fixed Codebook Gain                                           */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"
#include "codecParameters.h"

#include "testUtils.h"

#include "decodeGains.h"

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;

	/*** input and output buffers ***/
	/* input file: GA, GB, frameErasureFlag, fixedCodebookVector, adaptative codebook gain, fixed codebook gain */
	/* output : adaptative codebook gain, adaptative codebook gain */
	int16_t GA, GB;
	int16_t frameErasureFlag;
	word16_t fixedCodebookVector[L_SUBFRAME];
	word16_t adaptativeCodebookGain, fixedCodebookGain;

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

	initDecodeGains(decoderChannelContext);

	/* initialise buffers */

	/*** initialisation complete ***/


	/*** loop over input file ***/
	while(1) /* infinite loop, escape condition is in the reading of data */
	{
		int i;
		/* read the input data until we have some */
		if (fscanf(fpInput,"%hd",&GA) != 1) break;
		if (fscanf(fpInput,",%hd",&GB) != 1) break;
		if (fscanf(fpInput,",%hd",&frameErasureFlag) != 1) break;
		if (fscanf(fpInput,",%hd",&adaptativeCodebookGain) != 1) break;
		if (fscanf(fpInput,",%hd",&fixedCodebookGain) != 1) break;
		for (i=0; i<L_SUBFRAME; i++) {
			if (fscanf(fpInput,",%hd",&(fixedCodebookVector[i])) != 1) break;
		}
		
		/* call the tested funtion */
		decodeGains(decoderChannelContext, GA, GB, fixedCodebookVector, (uint8_t)frameErasureFlag,
				&adaptativeCodebookGain, &fixedCodebookGain);


		/* write the output to the output file */
		fprintf(fpOutput,"%d,%d\n",adaptativeCodebookGain, fixedCodebookGain);
	}
	exit (0);
}

