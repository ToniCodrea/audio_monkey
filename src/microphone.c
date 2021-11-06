#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "sound.h"
#include "al.h"
#include "alc.h"

ALCdevice* inputDevice;
int neutral_amplitude;
int quiet_amplitude;

extern int get_quiet_amplitude(void) {
	return quiet_amplitude;
}

extern int get_neutral_amplitude(void) {
	return neutral_amplitude;
}

extern void init_mic(void) {
	inputDevice = alcCaptureOpenDevice(NULL, FREQ, AL_FORMAT_MONO16, FREQ / 10); // request default capture device, half a second buffer  
	if (inputDevice == NULL) {
		perror("No input device found\n");
		exit(EXIT_FAILURE);
	}
	alcCaptureStart(inputDevice); // Begin capturing using input device 
}

void setup_mic(int* value) {
	clock_t start_t = clock();
	double amplitude = 0;
	int count = 0;
	while ( ((clock() - start_t) / CLOCKS_PER_SEC) < SETUP_TIME) {
		int new_amplitude = get_max_amplitude();
		if (new_amplitude > 0) {
			count++;
			amplitude = amplitude + ((new_amplitude - amplitude) / count);
		}
	}
	*value = amplitude;
 }

extern void setup_quiet(void) {
	setup_mic(&quiet_amplitude);
	printf("Quiet amplitued %d\n", quiet_amplitude);
}

extern void setup_neutral(void) {
	setup_mic(&neutral_amplitude);
	printf("Neutral amplitued %d\n", neutral_amplitude);
}

extern void setup_quiet_db(void) {
	quiet_amplitude = 70;
}

extern void setup_neutral_db(void) {
	neutral_amplitude = 100;
}

extern void close_mic(void) {
	// Stop capturing audio and close input device
	alcCaptureStop(inputDevice);
	alcCaptureCloseDevice(inputDevice);
}

extern int get_max_amplitude(void) {
	short buffer[CAP_SIZE]; // buffer to hold captured audio
	ALCint samples = 0; // number of samples to capture

	alcGetIntegerv(inputDevice, ALC_CAPTURE_SAMPLES, 1, &samples);

	int max_amplitude = 0;
	if (samples > CAP_SIZE) {
		alcCaptureSamples(inputDevice, buffer, CAP_SIZE); // capture samples
		// buffers might be of alternating sign so it might not matter
		for (int i = 1; i < CAP_SIZE; i++) {
			int difference = abs(abs(buffer[i]) - abs(buffer[i - 1]));
			if (difference > max_amplitude) {
				max_amplitude = difference;
			}
		}
	}
	return max_amplitude;
}

// scale is 0 ... quiet .... neutral .... 2x neutral ... 255
extern unsigned char get_level(void) {
	int level = get_max_amplitude();
	// small error
	if (level <= quiet_amplitude) {
		return 0;
	}
	unsigned char height = floor((255.0 * ((float)level / (float)(neutral_amplitude * 2))));
	return min(255, height);
}