#ifndef _H_AUDIO_
#define _H_AUDIO_

#include <stdint.h>
#include "pcm.h"

#define AUDIO_IO_MAX_CHANNELS 2
#define AUDIO_IO_MIXES 8
#define AUDIO_IO_OUTPUT_FRAMES 1024

#define AUDIO_IO_SUCCESS 0
#define AUDIO_IO_INVALIDPARAM -1
#define AUDIO_IO_ERROR -2

struct audio_io;
typedef struct audio_io audio_io_t;

struct audio_data {
	float *data[AUDIO_IO_MAX_CHANNELS];
	uint32_t frames;
};

typedef struct {
  uint64_t time;
  uint16_t pps;
  float rms[AUDIO_IO_MAX_CHANNELS];
  float peak[AUDIO_IO_MAX_CHANNELS];
} audio_io_realtime_data_t;

typedef void (*audio_output_callback_t)(struct audio_data *data, uint32_t frames, void *param);

typedef struct {
	const char *name;

	audio_format_t af;

	audio_output_callback_t callback;
	void *param;
} audio_output_info_t;

typedef void (*audio_input_callback_t)(struct audio_data data, uint32_t frames, void *param);

int audio_open(audio_io_t **audio, audio_output_info_t* output_info);
void audio_close(audio_io_t *audio);

#endif