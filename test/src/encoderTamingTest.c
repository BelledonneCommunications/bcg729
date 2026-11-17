/*
 * Copyright (c) 2026 Mustafa Gökçe.
 *
 * This file is part of bcg729.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*****************************************************************************/
/*                                                                           */
/* Test Program for the taming procedure of the encoder                      */
/*    Input: none, the signals are made by this program                      */
/*    Output: the tones lost, exit code 0 when none is                       */
/*                                                                           */
/* 200 histories of one to three parts of 0.3 to 4 s (a tone of 300 to       */
/* 3400 Hz, white noise or silence, at -30 to -3 dBFS), each followed by     */
/* 10 s of 800 Hz at -12 dBFS, go through a fresh encoder (VAD off) and a    */
/* fresh decoder. A tone is lost when the level of its windows of 100 ms,    */
/* from its sixth window on, spreads more than 6 dB (95th minus 5th          */
/* percentile). Without taming the encoder keeps a pitch gain above 1 on a   */
/* steady tone and the excitation of the decoder grows until its LP          */
/* synthesis overflows: 41 of these tones are lost with bcg729 1.1.2.        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include "bcg729/encoder.h"
#include "bcg729/decoder.h"

#define SAMPLE_RATE 8000
#define FRAME_LENGTH 80
#define WINDOW_LENGTH 800 /* 100 ms */
#define MAXIMUM_SAMPLES (20*SAMPLE_RATE)
#define TONE_SAMPLES (10*SAMPLE_RATE)
#define TONE_WINDOWS (TONE_SAMPLES/WINDOW_LENGTH - 5) /* the first five windows of the tone are left out */
#define HISTORIES 200
#define TONE_FREQUENCY 800.0
#define TONE_LEVEL -12.0 /* dBFS */
#define MAXIMUM_SPREAD 6.0 /* dB */
#define PI 3.14159265358979323846

static int16_t inputSignal[MAXIMUM_SAMPLES];
static int16_t outputSignal[MAXIMUM_SAMPLES];
static uint32_t randomState = 1;

/* a pseudo random value in [0, 1) */
static double uniform(void)
{
	randomState = randomState*1664525u + 1013904223u;
	return (double)(randomState>>8)/16777216.0;
}

static int compareLevels(const void *a, const void *b)
{
	const double x = *(const double *)a, y = *(const double *)b;
	return (x > y) - (x < y);
}

/* the spread in dB of the level of the tone in its windows of 100 ms */
static double toneSpread(const int16_t *tone)
{
	double levels[TONE_WINDOWS];
	int window, i;
	for (window=0; window<TONE_WINDOWS; window++) {
		const int16_t *samples = &(tone[(window+5)*WINDOW_LENGTH]);
		double real = 0.0, imaginary = 0.0;
		for (i=0; i<WINDOW_LENGTH; i++) {
			real += samples[i]*cos(2.0*PI*TONE_FREQUENCY*i/SAMPLE_RATE);
			imaginary -= samples[i]*sin(2.0*PI*TONE_FREQUENCY*i/SAMPLE_RATE);
		}
		levels[window] = 20.0*log10(sqrt(real*real + imaginary*imaginary)*2.0/WINDOW_LENGTH/32767.0 + 1e-12);
	}
	qsort(levels, TONE_WINDOWS, sizeof(double), compareLevels);
	return levels[TONE_WINDOWS*95/100] - levels[TONE_WINDOWS*5/100];
}

int main(void)
{
	int lostTones = 0;
	double largestSpread = 0.0;
	int history;

	for (history=0; history<HISTORIES; history++) {
		bcg729EncoderChannelContextStruct *encoderChannelContext;
		bcg729DecoderChannelContextStruct *decoderChannelContext;
		const int parts = 1 + (int)(uniform()*3);
		int toneStart, phase, part, length, i;
		double spread;

		/* the history */
		length = 0;
		for (part=0; part<parts; part++) {
			const int partLength = (int)((0.3 + uniform()*3.7)*SAMPLE_RATE)/FRAME_LENGTH*FRAME_LENGTH;
			const int kind = (int)(uniform()*3); /* 0: a tone, 1: white noise, 2: silence */
			const double amplitude = 32767.0*pow(10.0, (-30.0 + uniform()*27.0)/20.0);
			const double frequency = 300.0 + uniform()*3100.0;
			for (i=0; i<partLength && length<MAXIMUM_SAMPLES-TONE_SAMPLES; i++, length++) {
				inputSignal[length] = (int16_t)lrint(kind == 0 ? amplitude*sin(2.0*PI*frequency*i/SAMPLE_RATE)
					: kind == 1 ? amplitude*(uniform()*2.0 - 1.0)
					: 0.0);
			}
		}

		/* the tone */
		toneStart = length;
		phase = (int)(uniform()*1000);
		for (i=0; i<TONE_SAMPLES; i++, length++) {
			inputSignal[length] = (int16_t)lrint(32767.0*pow(10.0, TONE_LEVEL/20.0)*sin(2.0*PI*TONE_FREQUENCY*(i + phase)/SAMPLE_RATE));
		}

		/* encode and decode */
		encoderChannelContext = initBcg729EncoderChannel(0);
		decoderChannelContext = initBcg729DecoderChannel();
		for (i=0; i<length; i+=FRAME_LENGTH) {
			uint8_t bitStream[10];
			uint8_t bitStreamLength = 0;
			bcg729Encoder(encoderChannelContext, &(inputSignal[i]), bitStream, &bitStreamLength);
			bcg729Decoder(decoderChannelContext, bitStream, bitStreamLength, 0, 0, 0, &(outputSignal[i]));
		}
		closeBcg729EncoderChannel(encoderChannelContext);
		closeBcg729DecoderChannel(decoderChannelContext);

		spread = toneSpread(&(outputSignal[toneStart]));
		if (spread > largestSpread) {
			largestSpread = spread;
		}
		if (spread > MAXIMUM_SPREAD) {
			lostTones++;
			printf("history %d: the level of the %.0f Hz tone spreads %.1f dB\n", history, TONE_FREQUENCY, spread);
		}
	}
	printf("%d of %d tones lost, largest spread %.1f dB (at most %.1f dB)\n", lostTones, HISTORIES, largestSpread, MAXIMUM_SPREAD);

	if (lostTones > 0) {
		printf("FAIL: the encoder does not tame the pitch gain on steady tones\n");
		return EXIT_FAILURE;
	}
	printf("OK\n");
	return EXIT_SUCCESS;
}
