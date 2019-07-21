#ifndef _H_PCM_CONV_
#define _H_PCM_CONV_

#include <stdint.h>
#include "pcm.h"

size_t pcm_float_to_fixed(audio_format_t af, uint8_t* dst, float* src, size_t frames);
size_t pcm_fixed_to_float(audio_format_t af, float* dst, uint8_t* src, size_t frames);

#endif