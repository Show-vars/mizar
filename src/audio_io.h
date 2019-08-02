#ifndef _H_AUDIO_IO_
#define _H_AUDIO_IO_

#include <stdint.h>
#include "pcm.h"

#define AUDIO_IO_MAX_CHANNELS 2
#define AUDIO_IO_INPUTS 8
#define AUDIO_IO_BUSES 8
#define AUDIO_IO_OUTPUT_FRAMES 1024
#define AUDIO_PPS_SAMPLES 100

#define AUDIO_IO_SUCCESS 0
#define AUDIO_IO_INVALIDPARAM -1
#define AUDIO_IO_ERROR -2

struct audio_io;
typedef struct audio_io audio_io_t;

struct audio_data {
	float *data[AUDIO_IO_MAX_CHANNELS];
	uint32_t frames;
};

struct audio_io_realtime_data {
  uint64_t time;
  float pps;
  float rms[AUDIO_IO_BUSES][AUDIO_IO_MAX_CHANNELS];
  float peak[AUDIO_IO_BUSES][AUDIO_IO_MAX_CHANNELS];
};

typedef void (*audio_output_callback_t)(struct audio_data *data, uint32_t frames, void *param);

typedef struct {
	const char *name;

	audio_format_t af;

	int mix;

	audio_output_callback_t callback;
	void *param;
} audio_output_info_t;

typedef void (*audio_input_callback_t)(struct audio_data *data, uint32_t frames, void *param);

int audio_io_open(audio_io_t **audio, audio_output_info_t* output_info);
void audio_io_close(audio_io_t *audio);
void audio_io_get_realtime_data(audio_io_t *audio, struct audio_io_realtime_data *realtime_data);

#endif