#ifndef _H_AUDIO_CTRL_
#define _H_AUDIO_CTRL_

#include "audio_io.h"

#define AUDIO_CTRL_SUCCESS 0
#define AUDIO_CTRL_INVALIDPARAM -1
#define AUDIO_CTRL_ERROR -2

struct audio_ctrl;
typedef struct audio_ctrl audio_ctrl_t;

int audio_ctrl_open(audio_ctrl_t **audio_ctrl, audio_io_t *audio);
void audio_ctrl_close(audio_ctrl_t *audio_ctrl);

#endif