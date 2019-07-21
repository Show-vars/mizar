#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "pcm_conv.h"
#include "util_int128.h"
#include "util.h"

static void float_to_u8 (uint8_t *dst, const float *src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) {
		float f = src[i] * INT32_MAX;

		if (f >= INT32_MAX)
			dst[i] = UINT8_MAX;
		else if (f <= INT32_MIN)
			dst[i] = 0;
		else
			dst[i] = (unsigned int)((lrintf(f) >> 24) - INT8_MIN);
	}
}

static void float_to_s8 (uint8_t *dst, const float *src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) {
		float f = src[i] * INT32_MAX;

		if (f >= INT32_MAX)
			dst[i] = INT8_MAX;
		else if (f <= INT32_MIN)
			dst[i] = INT8_MIN;
		else
			dst[i] = lrintf(f) >> 24;
	}
}

static void float_to_u16 (uint8_t *dst, const float *src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) {
		uint16_t *out_val = (uint16_t *)(dst + i * sizeof (uint16_t));
		float f = src[i] * INT32_MAX;

		if (f >= INT32_MAX)
			*out_val = UINT16_MAX;
		else if (f <= INT32_MIN)
			*out_val = 0;
		else
			*out_val = (unsigned int)((lrintf(f) >> 16) - INT16_MIN);
	}
}

static void float_to_s16 (uint8_t *dst, const float *src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) {
		int16_t *out_val = (int16_t *)(dst + i * sizeof (int16_t));
		float f = src[i] * INT32_MAX;

		if (f >= INT32_MAX)
			*out_val = INT16_MAX;
		else if (f <= INT32_MIN)
			*out_val = INT16_MIN;
		else
			*out_val = lrintf(f) >> 16;
	}
}

static void float_to_u24 (uint8_t *dst, const float *src,const size_t samples) {

}

static void float_to_s24 (uint8_t *dst, const float *src, const size_t samples) {

}

static void float_to_u32 (uint8_t *dst, const float *src, const size_t samples) {
	const unsigned int U32_MAX = (1 << 24) - 1;
	const int S32_MAX = (1 << 23) - 1;
	const int S32_MIN = -(1 << 23);

	for (size_t i = 0; i < samples; i++) {
		uint32_t *out_val = (uint32_t *)(dst + i * sizeof (uint32_t));
		float f = src[i] * S32_MAX;

		if (f >= S32_MAX)
			*out_val = U32_MAX << 8;
		else if (f <= S32_MIN)
			*out_val = 0;
		else
			*out_val = (uint32_t)(lrintf(f) - S32_MIN) << 8;
	}
}

static void float_to_s32 (uint8_t *dst, const float *src, const size_t samples)
{
	const int S32_MAX = (1 << 23) - 1;
	const int S32_MIN = -(1 << 23);

	for (size_t i = 0; i < samples; i++) {
		int32_t *out_val = (int32_t *)(dst + i * sizeof (int32_t));
		float f = src[i] * S32_MAX;

		if (f >= S32_MAX)
			*out_val = S32_MAX << 8;
		else if (f <= S32_MIN)
			*out_val = S32_MIN * 256;
		else
			*out_val = lrintf(f) << 8;
	}
}

static void u8_to_float (float *dst, const uint8_t *src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) dst[i] = (((int)*src++) + INT8_MIN) / (float)(INT8_MAX + 1);
}

static void s8_to_float (float *dst, const uint8_t *src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) dst[i] = *src++ / (float)(INT8_MAX + 1);
}

static void u16_to_float (float *dst, const uint8_t *src, const size_t samples) {
	const uint16_t *in_16 = (uint16_t *)src;
	for (size_t i = 0; i < samples; i++) dst[i] = ((int)*in_16++ + INT16_MIN) / (float)(INT16_MAX + 1);
}

static void s16_to_float (float *dst, const uint8_t *src, const size_t samples) {
	const int16_t *in_16 = (int16_t *)src;
	for (size_t i = 0; i < samples; i++) dst[i] = *in_16++ / (float)(INT16_MAX + 1);
}

static void u24_to_float (float *dst, const uint8_t *src, const size_t samples) {
  // that will hurt
}

static void s24_to_float (float *dst, const uint8_t *src, const size_t samples) {
	// and this too
}

static void u32_to_float (float *dst, const uint8_t *src, const size_t samples) {
	const uint32_t *in_32 = (uint32_t *)src;
	for (size_t i = 0; i < samples; i++) dst[i] = ((float)*in_32++ + (float)INT32_MIN) / ((float)INT32_MAX + 1.0);
}

static void s32_to_float (float *dst, const uint8_t *src,  const size_t samples) {
	const int32_t *in_32 = (int32_t *) src;
	for (size_t i = 0; i < samples; i++) dst[i] = *in_32++ / ((float)INT32_MAX + 1.0);
}

static void swap_sign_8(uint8_t* src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) *src++ ^= 1 << 7;
}

static void swap_sign_16(uint16_t* src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) *src++ ^= 1 << 15;
}

static void swap_sign_24(uint8_t* src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) src[i * 3] ^= 1 << 15; // bad impl
}

static void swap_sign_32(uint32_t* src, const size_t samples) {
	for (size_t i = 0; i < samples; i++) *src++ ^= 1 << 31;
}

static int swap_sign(audio_format_t af, uint8_t *src, const size_t frames) {
	switch (af_get_format(af)) {
    case SF_FORMAT_U8:
		case SF_FORMAT_S8: 
      swap_sign_8((uint8_t *)src, frames * af_get_channels(af));
      return 0;
		case SF_FORMAT_U16:
		case SF_FORMAT_S16: 
      swap_sign_16((uint16_t *)src, frames * af_get_channels(af));
      return 0;
		case SF_FORMAT_U24:
		case SF_FORMAT_S24:
      swap_sign_24((uint8_t *)src, frames * af_get_channels(af));
			return 0;
		case SF_FORMAT_U32:
		case SF_FORMAT_S32:
      swap_sign_32((uint32_t *)src, frames * af_get_channels(af));
			return 0;
		case SF_FORMAT_FLOAT:
		default:
    return -1;
	}
}

static void swap_endian_16(int16_t* src, const size_t samples) {
  for (size_t i = 0; i < samples; i++) src[i] = BSWAP_16(src[i]);
}

static void swap_endian_24(int8_t* src, const size_t samples) {
  for (size_t i = 0; i < samples; i++) src[i] = BSWAP_24(src[i] | src[i + 1] << 8 | src[i + 2] << 16); // bad impl
}

static void swap_endian_32(int32_t* src, const size_t samples) {
  for (size_t i = 0; i < samples; i++) src[i] = BSWAP_32(src[i]);
}

static int swap_endian (audio_format_t af, uint8_t* src, size_t frames) {
  uint8_t fmt = af_get_format(af);

	if (fmt == SF_FORMAT_U8 || fmt == SF_FORMAT_S8)	return -1;

	switch (fmt) {
		case SF_FORMAT_U16:
		case SF_FORMAT_S16: 
      swap_endian_16((int16_t *)src, frames * af_get_channels(af));
      return 0;
		case SF_FORMAT_U24:
		case SF_FORMAT_S24:
      swap_endian_24((int8_t *)src, frames * af_get_channels(af));
			return 0;
		case SF_FORMAT_U32:
		case SF_FORMAT_S32:
		case SF_FORMAT_FLOAT:
      swap_endian_32((int32_t *)src, frames * af_get_channels(af));
			return 0;
		default:
      return -1;
	}
}

size_t pcm_float_to_fixed(audio_format_t af, uint8_t* dst, float* src, size_t frames) {
  switch(af_get_format(af)) {
    case SF_FORMAT_U8:
      float_to_u8(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S8:
      float_to_s8(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_U16:
      float_to_u16(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S16:
      float_to_s16(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_U24:
      float_to_u24(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S24:
      float_to_s24(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_U32:
      float_to_u32(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S32:
      float_to_s32(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_FLOAT:
    default:
      return 0;
  }

  if(af_get_endian(af) != AF_NATIVE_ENDIAN) {
    swap_endian(af, dst, frames);
  }

  return frames;
}

size_t pcm_fixed_to_float(audio_format_t af, float* dst, uint8_t* src, size_t frames) {
  if(af_get_endian(af) != AF_NATIVE_ENDIAN) {
    swap_endian(af, src, frames);
  }
  
  switch(af_get_format(af)) {
    case SF_FORMAT_U8:
      u8_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S8:
      s8_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_U16:
      u16_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S16:
      s16_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_U24:
      u24_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S24:
      s24_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_U32:
      u32_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_S32:
      s32_to_float(dst, src, frames * af_get_channels(af));
      break;
    case SF_FORMAT_FLOAT:
    default:
      return 0;
  }

  return frames;
}