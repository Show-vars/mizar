#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include "audio.h"
#include "util/mem.h"

#define AUDIO_IO_STATE_CLOSED 0
#define AUDIO_IO_STATE_OPENED 1

struct audio_mix {
	float buffer[AUDIO_IO_MAX_CHANNELS][AUDIO_IO_OUTPUT_FRAMES];
};

struct audio_input {
	audio_input_callback_t callback;
	void *param;
};

struct audio_output {
	audio_output_callback_t callback;
	void *param;
};

struct audio_io {
	pthread_t thread;

	bool initialized;
  atomic_int state;

	uint8_t channels;
	uint32_t framerate;

	struct audio_input  input[AUDIO_IO_MIXES];
	struct audio_mix    mixes[AUDIO_IO_MIXES];
	struct audio_output output;
};

//
// Audio thread functions
//

static void audio_input_output(struct audio_io *audio) {
	struct audio_data data[AUDIO_IO_MIXES];

	memset(data, 0, sizeof(data));

	for(int mix_idx = 0; mix_idx < AUDIO_IO_MIXES; mix_idx++) {
		struct audio_mix *mix = &audio->mixes[mix_idx];
		
		memset(mix->buffer[0], 0, AUDIO_IO_OUTPUT_FRAMES * AUDIO_IO_MAX_CHANNELS * sizeof(float));

		for(uint8_t ch = 0; ch < AUDIO_IO_MAX_CHANNELS; ch++) {
			data[mix_idx].data[ch] = mix->buffer[ch];
		}
	}

	audio->output.callback(&data[0], AUDIO_IO_OUTPUT_FRAMES, audio->output.param);
}

static void *audio_thread(void *param) {
	struct audio_io *audio = param;

	while(audio->state == AUDIO_IO_STATE_OPENED) {
		audio_input_output(audio);
	}

	return NULL;
}

//
// Public functions
//

int audio_open(audio_io_t **audio, audio_output_info_t* output_info) {
	if(af_get_channels(output_info->af) > AUDIO_IO_MAX_CHANNELS)
		return AUDIO_IO_INVALIDPARAM;
	
	struct audio_io *io;

	if (!(io = zalloc(sizeof(struct audio_io))))
		goto fail;
	
	io->channels = af_get_channels(output_info->af);
	io->framerate = af_get_rate(output_info->af);

	io->output.callback = output_info->callback;
	io->output.param = output_info->param;
	
	io->state = AUDIO_IO_STATE_OPENED;

	if (pthread_create(&io->thread, NULL, audio_thread, io) != 0)
		goto fail;

	io->initialized = true;
	*audio = io;
	return AUDIO_IO_SUCCESS;

	fail:
	audio_close(io);
	return AUDIO_IO_ERROR;
}

void audio_close(audio_io_t *audio) {
	if (!audio) return;

	if (audio->initialized) {
		audio->state = AUDIO_IO_STATE_CLOSED;
		pthread_join(audio->thread, NULL);
	}

	free(audio);
}