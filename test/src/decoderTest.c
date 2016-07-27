/*
 decoderTest.c

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
/* Test Program for decoder                                                  */
/*    Input: 15 parameters and the frame erasure flag on each row of a       */
/*           a text CSV file.                                                */
/*    Ouput: the reconstructed signal : each frame (80 16 bits PCM values)   */
/*           on a row of a text CSV file                                     */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <time.h>


#include "typedef.h"
#include "codecParameters.h"
#include "utils.h"

#include "testUtils.h"

#include "bcg729/decoder.h"


int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;
	FILE *fpOutput;
	FILE *fpBinOutput;

	/*** input and output buffers ***/
	uint16_t inputBuffer[NB_PARAMETERS+1]; /* input buffer: an array containing the 15 parameters and the frame erasure flag */
	uint8_t bitStream[10]; /* binary input for the decoder */
	int16_t outputBuffer[L_FRAME]; /* output buffer: the reconstructed signal */ 

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
	sprintf(outputFile, "%s.pcm",filePrefix);
	if ( (fpBinOutput = fopen(outputFile, "wb")) == NULL) {
		printf("%s - Error: can't create file  %s\n", argv[0], outputFile);
		exit(-1);
	}
	
	/*** init of the tested bloc ***/
	bcg729DecoderChannelContextStruct *decoderChannelContext = initBcg729DecoderChannel();

	/*** initialisation complete ***/
	/* perf measurement */
	clock_t start, end;
	double cpu_time_used=0.0;
	int framesNbr =0;
/* increase LOOP_N to increase input length and perform a more accurate profiling or perf measurement */
#define LOOP_N 1
	int j;
	for (j=0; j<LOOP_N; j++) {
	/* perf measurement */
		/*** loop over input file ***/
		while(fscanf(fpInput, "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd", &(inputBuffer[0]), &(inputBuffer[1]), &(inputBuffer[2]), &(inputBuffer[3]), &(inputBuffer[4]), &(inputBuffer[5]), &(inputBuffer[6]), &(inputBuffer[7]), &(inputBuffer[8]), &(inputBuffer[9]), &(inputBuffer[10]), &(inputBuffer[11]), &(inputBuffer[12]), &(inputBuffer[13]), &(inputBuffer[14]), &(inputBuffer[15]))==16) /* index 4 and 5 are inverted to get P0 in 4 and P1 in 5 in the array */
		{ /* input buffer contains the parameters and in [15] the frame erasure flag */
			int i;
			framesNbr++;

			parametersArray2BitStream(inputBuffer, bitStream);

			start = clock();
			bcg729Decoder(decoderChannelContext, bitStream, 10, inputBuffer[15], 0, 0, outputBuffer);
			end = clock();

			cpu_time_used += ((double) (end - start));

			/* write the output to the output files (only on first loop of perf measurement)*/
			if (j==0) {
				fprintf(fpOutput,"%d",outputBuffer[0]);
				for (i=1; i<L_FRAME; i++) {
					fprintf(fpOutput,",%d",outputBuffer[i]);
				}
				fprintf(fpOutput,"\n");
				/* write the ouput to raw data file */
				fwrite(outputBuffer, sizeof(int16_t), L_FRAME, fpBinOutput);
			}

		}
	/* perf measurement */
		rewind(fpInput);
	}

	closeBcg729DecoderChannel(decoderChannelContext);

/* Perf measurement: uncomment next line to print cpu usage */
	printf("Decode %d frames in %f seconds : %f us/frame\n", framesNbr, cpu_time_used/CLOCKS_PER_SEC, cpu_time_used*1000000/((double)framesNbr*CLOCKS_PER_SEC));
	/* perf measurement */
	exit (0);
}

