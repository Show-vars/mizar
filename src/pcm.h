#ifndef _H_PCM_
#define _H_PCM_

#include <stdint.h>
#include "util.h"
/*
  Audio format pattern (32-bit unsigned):
    Bit    Size   Desc
    0      1      endian   (0 little, 1 big)
    1      1      signed   (0-1)
    2      1      reserved
    3-5    3      depth    (0-7: 8, 16, 24, 32, ...)
    6-23   18     rate     (0-262143)
    24-31  8      channels (0-255)
*/

typedef uint32_t audio_format_t;

#define MAX_AUDIO_CHANNELS 2

#define AF_ENDIAN_MASK    0x00000001
#define AF_SIGNED_MASK    0x00000002
#define AF_DEPTH_MASK     0x00000038
#define AF_RATE_MASK      0x00ffffc0
#define AF_CHANNELS_MASK  0xff000000

#define AF_ENDIAN_SHIFT   0
#define AF_SIGNED_SHIFT   1
#define AF_DEPTH_SHIFT    0
#define AF_RATE_SHIFT     6
#define AF_CHANNELS_SHIFT 24

#define af_get_endian(af)       ( ((af) & AF_ENDIAN_MASK      ) >> AF_ENDIAN_SHIFT )
#define af_get_signed(af)       ( ((af) & AF_SIGNED_MASK      ) >> AF_SIGNED_SHIFT )
#define af_get_depth(af)        ( ((af) & AF_DEPTH_MASK       ) >> AF_DEPTH_SHIFT )
#define af_get_rate(af)         ( ((af) & AF_RATE_MASK        ) >> AF_RATE_SHIFT )
#define af_get_channels(af)     ( ((af) & AF_CHANNELS_MASK    ) >> AF_CHANNELS_SHIFT )

#define af_endian(val)          ( ((val) << AF_ENDIAN_SHIFT   ) & AF_ENDIAN_MASK )
#define af_signed(val)          ( ((val) << AF_SIGNED_SHIFT   ) & AF_SIGNED_MASK )
#define af_depth(val)           ( ((val) << AF_DEPTH_SHIFT    ) & AF_DEPTH_MASK )
#define af_rate(val)            ( ((val) << AF_RATE_SHIFT     ) & AF_RATE_MASK )
#define af_channels(val)        ( ((val) << AF_CHANNELS_SHIFT ) & AF_CHANNELS_MASK )

#define af_get_sample_size(af)  ( af_get_depth((af))       >> 3 )
#define af_get_frame_size(af)   ( af_get_sample_size((af)) * af_get_channels((af)) )
#define af_get_second_size(af)  ( af_get_rate((af))        * af_get_frame_size((af)) )

typedef struct  {
  uint8_t channels;
  uint16_t sample_rate;
  size_t frames;
  float* data;
} audio_data_t;

typedef struct  {
  audio_format_t af;
  size_t frames;
  size_t size;
  uint8_t* data;
} audio_io_data_t;

static inline int16_t pcm_clamp_sample_int16(float sample) {
  return clamp((int32_t)(sample * INT16_MAX), -INT16_MAX, INT16_MAX);
}

static inline int pcm_fti(audio_io_data_t dst, audio_data_t src) {
  if(af_get_rate(dst.af) != src.sample_rate) return -1;
  if(dst.frames != src.frames) return -2;

  for(size_t fr = 0; fr < src.frames; fr++) {
    for(size_t ch = 0; ch < src.channels; ch++) {
      *((int16_t*)dst.data + fr*af_get_sample_size(dst.af) + ch) = pcm_clamp_sample_int16(*(src.data + fr + ch));
    }
  }

  return 0;
}
#endif