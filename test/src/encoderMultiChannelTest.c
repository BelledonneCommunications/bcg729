/*
 encoderMultiChannelTest.c

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
/* Test Program for encoder on multichannels                                 */
/*    Input: the reconstructed signal : each frame (80 16 bits PCM values)   */
/*           on a row of a text CSV file                                     */
/*    Output: 15 parameters on each row of a text CSV file.                  */
/*                                                                           */
/*    All arguments shall be filenames for input file                        */
/*    output file keep the prefix and change the file extension to .out.multi*/
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

#define MAX_CHANNEL_NBR 50
int main(int argc, char *argv[] )
{
	int i,j,k;

	/*** get calling argument ***/
  	char *filePrefix[MAX_CHANNEL_NBR];
	getArgumentsMultiChannel(argc, argv, filePrefix); /* check argument and set filePrefix if needed */

	/*** input and output file pointers ***/
	FILE *fpInput[MAX_CHANNEL_NBR];
	FILE *fpOutput[MAX_CHANNEL_NBR];

	/*** input and output buffers ***/
	int16_t inputBuffer[L_FRAME]; /* output buffer: the signal */ 
	uint16_t outputBuffer[NB_PARAMETERS]; /* output buffer: an array containing the 15 parameters */
	uint8_t bitStream[10]; /* binary output of the encoder */
	bcg729EncoderChannelContextStruct *encoderChannelContext[MAX_CHANNEL_NBR]; /* context, one per channel */
	uint16_t inputIsBinary[MAX_CHANNEL_NBR]; /* store the information on each input file format (CVS or binary PCM) */


	/*** inits ***/
	/* open the input file */
	for (i=0; i<argc-1; i++) {
		inputIsBinary[i] = 0;
		if (argv[i+1][strlen(argv[i+1])-1] == 'n') { /* input filename and by n, it's probably a .in : CSV file */
			if ( (fpInput[i] = fopen(argv[i+1], "r")) == NULL) {
				printf("%s - Error: can't open file  %s\n", argv[0], argv[i+1]);
				exit(-1);
			}
		} else { /* it's probably a binary file */
			inputIsBinary[i] = 1;
			if ( (fpInput[i] = fopen(argv[i+1], "rb")) == NULL) {
				printf("%s - Error: can't open file  %s\n", argv[0], argv[i+1]);
				exit(-1);
			}
		}


		/* create the output file(filename is the same than input file with the .out extension) */
		char *outputFile = malloc((strlen(filePrefix[i])+15)*sizeof(char));
		sprintf(outputFile, "%s.out.multi",filePrefix[i]);
		if ( (fpOutput[i] = fopen(outputFile, "w")) == NULL) {
			printf("%s - Error: can't create file  %s\n", argv[0], outputFile);
			exit(-1);
		}

		/*** init of the tested bloc ***/
		encoderChannelContext[i] = initBcg729EncoderChannel(0);
	}
	

	/*** initialisation complete ***/
	/* perf measurement */
	clock_t start, end;
	double cpu_time_used=0.0;
	int framesNbr =0;
/* increase LOOP_N to increase input length and perform a more accurate profiling or perf measurement */
#define LOOP_N 1
	for (j=0; j<LOOP_N; j++) {
	/* perf measurement */

		/*** loop over inputs file ***/
		int endedFilesNbr = 0; 
		int endedFiles[MAX_CHANNEL_NBR]; 
		for (k=0; k<argc-1; k++) { /* reset the array of boolean containing a flag for files already read */
			endedFiles[k]=0;
		}
		while (endedFilesNbr<argc-1) { /* loop until the longest file is over */
			for (k=0; k<argc-1; k++) { /* read one frame on each not ended file */
				if (endedFiles[k]==0) { /* read only if the file is not over */



					while(1) /* infinite loop, escape condition is in the reading of data */
					{
						int i;
						uint8_t bitStreamLength;

						/* read the input data until we have some */
						if (inputIsBinary[k]) {
							if (fread(inputBuffer, sizeof(int16_t), L_FRAME, fpInput[k]) != L_FRAME) break;
						} else {
							if (fscanf(fpInput[k],"%hd",&(inputBuffer[0])) != 1) break;
							for (i=1; i<L_FRAME; i++) {
								if (fscanf(fpInput[k],",%hd",&(inputBuffer[i])) != 1) break;
							}
						}

						framesNbr++;
						start = clock();

						bcg729Encoder(encoderChannelContext[k], inputBuffer, bitStream, &bitStreamLength);
			
						end = clock();
						cpu_time_used += ((double) (end - start));
						
						if (bitStreamLength == 10) {
							/* convert bitStream output in an array for easier debug */
							parametersBitStream2Array(bitStream, outputBuffer);

							if (j==0) {
								/* write the output to the output file */
								fprintf(fpOutput[k],"%d",outputBuffer[0]);
								for (i=1; i<NB_PARAMETERS; i++) {
									fprintf(fpOutput[k],",%d",outputBuffer[i]);
								}
								fprintf(fpOutput[k],",0\n");
							}
						}
					} 
					/* we've reach the end of the file */
					endedFiles[k]=1;
					endedFilesNbr++;
				}
			}
		}

		/* perf measurement */
		for (k=0; k<argc-1; k++) {
			rewind(fpInput[k]);
		}
	}

	/* close encoder channels */
	for (k=0; k<argc-1; k++) {
		closeBcg729EncoderChannel(encoderChannelContext[k]);
	}
/* Perf measurement: uncomment next line to print cpu usage */
	printf("Encode %d frames in %f seconds : %f us/frame\n", framesNbr, cpu_time_used/CLOCKS_PER_SEC, cpu_time_used*1000000/((double)framesNbr*CLOCKS_PER_SEC));
	/* perf measurement */

	exit (0);
}

