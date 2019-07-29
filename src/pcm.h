#ifndef _H_PCM_
#define _H_PCM_

#include <stdint.h>
#include <math.h>
#include "util/int128.h"
#include "config.h"

/*
  Audio format pattern (32-bit unsigned):
    Bit    Size   Desc
    0      1      endian   (0 little, 1 big)
    1-5    5      format   (0-32 SF_FORMAT_* family values)
    6-23   18     rate     (0-262143)
    24-31  8      channels (0-255)
*/

#ifdef WORDS_BIGENDIAN
  #define AF_NATIVE_ENDIAN 0x01
#else
  #define AF_NATIVE_ENDIAN 0x00
#endif

typedef uint32_t audio_format_t;

#define SF_FORMAT_S8    0x01 /* 8-bit signed */
#define SF_FORMAT_U8    0x02 /* 8-bit unsigned */
#define SF_FORMAT_S16   0x03 /* 16-bit signed */
#define SF_FORMAT_U16   0x04 /* 16-bit unsigned */
#define SF_FORMAT_S24   0x05 /* 24-bit signed */
#define SF_FORMAT_U24   0x06 /* 24-bit unsigned */
#define SF_FORMAT_S32   0x07 /* 24-bit signed */
#define SF_FORMAT_U32   0x08 /* 24-bit unsigned */
#define SF_FORMAT_FLOAT 0x09 /* float in range -1.0 to 1.0 */

#define AF_ENDIAN_MASK    0x00000001
#define AF_FORMAT_MASK    0x0000003e
#define AF_RATE_MASK      0x00ffffc0
#define AF_CHANNELS_MASK  0xff000000

#define AF_ENDIAN_SHIFT   0
#define AF_FORMAT_SHIFT   1
#define AF_RATE_SHIFT     6
#define AF_CHANNELS_SHIFT 24

#define af_get_endian(af)       ( ((af) & AF_ENDIAN_MASK      ) >> AF_ENDIAN_SHIFT )
#define af_get_format(af)       ( ((af) & AF_FORMAT_MASK      ) >> AF_FORMAT_SHIFT )
#define af_get_rate(af)         ( ((af) & AF_RATE_MASK        ) >> AF_RATE_SHIFT )
#define af_get_channels(af)     ( ((af) & AF_CHANNELS_MASK    ) >> AF_CHANNELS_SHIFT )

#define af_endian(val)          ( ((val) << AF_ENDIAN_SHIFT   ) & AF_ENDIAN_MASK )
#define af_format(val)          ( ((val) << AF_FORMAT_SHIFT   ) & AF_FORMAT_MASK )
#define af_rate(val)            ( ((val) << AF_RATE_SHIFT     ) & AF_RATE_MASK )
#define af_channels(val)        ( ((val) << AF_CHANNELS_SHIFT ) & AF_CHANNELS_MASK )

static inline uint8_t af_get_signed(audio_format_t af) {
  switch(af_get_format(af)) {
    default:
    case SF_FORMAT_U8: 
    case SF_FORMAT_U16:
    case SF_FORMAT_U24:
    case SF_FORMAT_U32:
    case SF_FORMAT_FLOAT: return 0;
    case SF_FORMAT_S8:
    case SF_FORMAT_S16:
    case SF_FORMAT_S24:
    case SF_FORMAT_S32: return 1;
  }
}

static inline uint8_t af_get_depth(audio_format_t af) {
  switch(af_get_format(af)) {
    default:
    case SF_FORMAT_S8:
    case SF_FORMAT_U8: return 8;
    case SF_FORMAT_S16:
    case SF_FORMAT_U16: return 16;
    case SF_FORMAT_S24:
    case SF_FORMAT_U24: return 24;
    case SF_FORMAT_S32:
    case SF_FORMAT_U32:
    case SF_FORMAT_FLOAT: return 32;
  }
}

static inline uint8_t af_get_sample_size(audio_format_t af) {
  switch(af_get_format(af)) {
    default:
    case SF_FORMAT_S8:
    case SF_FORMAT_U8: return 1;
    case SF_FORMAT_S16:
    case SF_FORMAT_U16: return 2;
    case SF_FORMAT_S24:
    case SF_FORMAT_U24: return 3;
    case SF_FORMAT_S32:
    case SF_FORMAT_U32:
    case SF_FORMAT_FLOAT: return 4;
  }
}

#define af_get_frame_size(af)	    ( af_get_sample_size((af)) * af_get_channels((af)) )
#define af_get_second_size(af)    ( af_get_rate((af))        * af_get_frame_size((af)) )

#define pcm_sample_sign_8(sample)  ((sample) ^ 1 << 7)
#define pcm_sample_sign_16(sample) ((sample) ^ 1 << 15)
#define pcm_sample_sign_24(sample) ((sample) ^ 1 << 23)
#define pcm_sample_sign_32(sample) ((sample) ^ 1 << 31)

static inline uint64_t pcm_frames_to_ns(int sample_rate, uint64_t frames) {
	util_uint128_t v;
	v = util_mul_64_64(frames, 1000000000ULL);
	v = util_div_128_64(v, (uint64_t)sample_rate);
	return v.l;
}

static inline uint64_t pcm_ns_to_frames(int sample_rate, uint64_t ns) {
	util_uint128_t v;
	v = util_mul_64_64(ns, sample_rate);
	v = util_div_128_64(v, 1000000000ULL);
	return v.l;
}

static inline float pcm_to_db(const float m) {
	return (m == 0.0f) ? -INFINITY : (20.0f * log10f(m));
}

#endif