/*
 encoderTest.c

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
/* Test Program for encoder                                                  */
/*    Input: the reconstructed signal : each frame (80 16 bits PCM values)   */
/*           on a row of a text CSV file                                     */
/*    Output: 15 parameters on each row of a text CSV file.                  */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>


#include "typedef.h"
#include "codecParameters.h"
#include "utils.h"

#include "testUtils.h"

#include "bcg729/encoder.h"

FILE *fpOutput;

int main(int argc, char *argv[] )
{
	/*** get calling argument ***/
  	char *filePrefix;
	getArgument(argc, argv, &filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput;

	/*** input and output buffers ***/
	int16_t inputBuffer[L_FRAME]; /* output buffer: the signal */ 
	uint8_t bitStream[10]; /* binary output of the encoder */
	uint16_t outputBuffer[NB_PARAMETERS]; /* output buffer: an array containing the 15 parameters */

	/*** inits ***/
	/* open the input file */
	uint16_t inputIsBinary = 0;
	if (argv[1][strlen(argv[1])-1] == 'n') { /* input filename and by n, it's probably a .in : CSV file */
		if ( (fpInput = fopen(argv[1], "r")) == NULL) {
			printf("%s - Error: can't open file  %s\n", argv[0], argv[1]);
			exit(-1);
		}
	} else { /* it's probably a binary file */
		inputIsBinary = 1;
		if ( (fpInput = fopen(argv[1], "rb")) == NULL) {
			printf("%s - Error: can't open file  %s\n", argv[0], argv[1]);
			exit(-1);
		}
	}

	/* create the output file(filename is the same than input file with the .out extension) */
	char *outputFile = malloc((strlen(filePrefix)+5)*sizeof(char));
	sprintf(outputFile, "%s.out",filePrefix);
	if ( (fpOutput = fopen(outputFile, "w")) == NULL) {
		printf("%s - Error: can't create file  %s\n", argv[0], outputFile);
		exit(-1);
	}
	
	/*** init of the tested bloc ***/
	bcg729EncoderChannelContextStruct *encoderChannelContext = initBcg729EncoderChannel(1);

	/*** initialisation complete ***/
	/* perf measurement */
	clock_t start, end;
	double cpu_time_used=0.0f;
	int framesNbr =0;
/* increase LOOP_N to increase input length and perform a more accurate profiling or perf measurement */
#define LOOP_N 1
	int j;
	for (j=0; j<LOOP_N; j++) {
	/* perf measurement */

		/*** loop over input file ***/
		while(1) /* infinite loop, escape condition is in the reading of data */
		{
			int i;
			uint8_t bitStreamLength;

			/* read the input data until we have some */
			if (inputIsBinary) {
				if (fread(inputBuffer, sizeof(int16_t), L_FRAME, fpInput) != L_FRAME) break;
			} else {
				if (fscanf(fpInput,"%hd",&(inputBuffer[0])) != 1) break;
				for (i=1; i<L_FRAME; i++) {
					if (fscanf(fpInput,",%hd",&(inputBuffer[i])) != 1) break;
				}
			}


			framesNbr++;
			start = clock();

			bcg729Encoder(encoderChannelContext, inputBuffer, bitStream, &bitStreamLength);

			end = clock();
			cpu_time_used += ((double) (end - start));


			/* convert bitStream output in an array for easier debug */
			if (bitStreamLength == 10) {
				parametersBitStream2Array(bitStream, outputBuffer);

				if (j==0) {
					/* write the output to the output file */
					fprintf(fpOutput,"%d",outputBuffer[0]);
					for (i=1; i<NB_PARAMETERS; i++) {
						fprintf(fpOutput,",%d",outputBuffer[i]);
					}
					fprintf(fpOutput,",0,1\n");
				}
			} else if (bitStreamLength == 2) {
					/* write the output to the output file */
					fprintf(fpOutput,"%d,%d,%d,%d",(bitStream[0]>>7)&0x01, (bitStream[0]>>2)&0x1F, ((bitStream[0]&0x03)<<2) | ((bitStream[1]>>6)&0x03), ((bitStream[1])>>1)&0x1F);
					for (i=4; i<NB_PARAMETERS; i++) {
						fprintf(fpOutput,",0");  /* just get the correct number of parameters */
					}
					fprintf(fpOutput,",0,2\n");
			} else { /* bitstream to 0, un transmitted frame */
					for (i=0; i<NB_PARAMETERS; i++) {
						fprintf(fpOutput,"0,");  /* just get the correct number of parameters */
					}
					fprintf(fpOutput,"1,0\n");
			}
		}
		/* perf measurement */
		rewind(fpInput);
	}
	closeBcg729EncoderChannel(encoderChannelContext);
/* Perf measurement: uncomment next line to print cpu usage */
	printf("Encode %d frames in %f seconds : %f us/frame\n", framesNbr, cpu_time_used/CLOCKS_PER_SEC, cpu_time_used*1000000/((double)framesNbr*CLOCKS_PER_SEC));
	/* perf measurement */

	exit (0);
}

