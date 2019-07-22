#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "audiobuffer.h"
#include "pcm.h"
#include "util.h"

audiobuffer_t* audiobuffer_create(audio_format_t af, size_t frames) {
  audiobuffer_t* b;

  size_t capacity = frames * af_get_channels(af);
  b = mmalloc(offsetof(audiobuffer_t, data) + capacity * sizeof(float));
  
  b->af = af;
  b->capacity = capacity;
  b->frames = frames;
  b->available = b->read_index = b->write_index = 0;

  uv_mutex_init_recursive(&b->mutex);
  
  return b;
}

void audiobuffer_destroy(audiobuffer_t* b) {
  uv_mutex_destroy(&b->mutex);
  
  free(b);
}

void audiobuffer_reset(audiobuffer_t* b) {
  uv_mutex_lock(&b->mutex);
  b->available = b->read_index = b->write_index = 0;
  b->frames = 0;
  uv_mutex_unlock(&b->mutex);
}

uint64_t audiobuffer_get_frames(audiobuffer_t* b) {
  uint64_t frames;
  
  uv_mutex_lock(&b->mutex);
  frames = b->frames;
  uv_mutex_unlock(&b->mutex);

  return frames;
}

void audiobuffer_set_frames(audiobuffer_t* b, uint64_t frames) {
  uv_mutex_lock(&b->mutex);
  b->frames = frames;
  uv_mutex_unlock(&b->mutex);
}

size_t audiobuffer_read_begin(audiobuffer_t* b, const size_t max_frames) {
  if(b->r_begin || max_frames == 0) return 0;

  size_t channels = af_get_channels(b->af);

  b->r_begin = 1;
  b->r_max_samples = max_frames * channels;
  b->r_index = b->read_index;
  b->r_count = 0;

  uv_mutex_lock(&b->mutex);
    b->r_available = b->available;
  uv_mutex_unlock(&b->mutex);

  b->r_available = min(b->r_available, b->r_max_samples);

  return b->r_available / af_get_channels(b->af);
}

size_t audiobuffer_read(audiobuffer_t* b, float** ptr) {
  if(!b->r_begin) return 0;

  size_t channels = af_get_channels(b->af);
  size_t samples = b->r_max_samples - b->r_count;
  float* src = b->data;

  if (b->r_available == 0) return 0;

  size_t limit = min(b->r_index + b->r_available, b->capacity);
  size_t count = min(limit - b->r_index, samples);
  
  *ptr = &src[b->r_index];

  return count / channels;
}

size_t audiobuffer_read_consume(audiobuffer_t* b, const size_t frames) {
  if(!b->r_begin) return 0;

  size_t channels = af_get_channels(b->af);
  size_t samples = frames * channels;

  b->r_available = b->r_available - samples;
  b->r_index = (b->r_index + samples) % b->capacity;
  b->r_count = b->r_count + samples;

  return b->r_count / channels;
}

size_t audiobuffer_read_end(audiobuffer_t* b) {
  if(!b->r_begin) return 0;

  b->r_begin = 0;

  if(b->r_count == 0) return 0;

  size_t channels = af_get_channels(b->af);

  b->read_index = (b->read_index + b->r_count) % b->capacity;

  uv_mutex_lock(&b->mutex);
    b->available -= b->r_count;
    b->frames += b->r_count / channels;
  uv_mutex_unlock(&b->mutex);

  return b->r_count / channels;
}
size_t audiobuffer_write_begin(audiobuffer_t* b, const size_t max_frames) {
  if(b->w_begin || max_frames == 0) return 0;

  size_t channels = af_get_channels(b->af);

  b->w_begin = 1;
  b->w_max_samples = max_frames * channels;
  b->w_index = b->write_index;
  b->w_count = 0;

  uv_mutex_lock(&b->mutex);
    b->w_available = b->capacity - b->available;
  uv_mutex_unlock(&b->mutex);

  b->w_available = min(b->w_available, b->w_max_samples);

  return b->w_available / channels;
}

size_t audiobuffer_write(audiobuffer_t* b, float** ptr) {
  if(!b->w_begin) return 0;
  
  size_t channels = af_get_channels(b->af);
  size_t samples = b->w_max_samples - b->w_count;
  float* dst = b->data;

  if (b->w_available == 0) return 0;

  size_t limit = min(b->w_index + b->w_available, b->capacity);
  size_t count = min(limit - b->w_index, samples);
  
  *ptr = &dst[b->w_index];

  return count / channels;
}

size_t audiobuffer_write_fill(audiobuffer_t* b, const size_t frames) {
  if(!b->w_begin || frames == 0) return 0;

  size_t channels = af_get_channels(b->af);
  size_t samples = frames * channels;

  b->w_available = b->w_available - samples;
  b->w_index = (b->w_index + samples) % b->capacity;
  b->w_count = b->w_count + samples;

  return b->w_count / channels;
}

size_t audiobuffer_write_end(audiobuffer_t* b) {
  if(!b->w_begin) return 0;

  b->w_begin = 0;

  if(b->w_count == 0) return 0;

  size_t channels = af_get_channels(b->af);

  b->write_index = (b->write_index + b->w_count) % b->capacity;

  uv_mutex_lock(&b->mutex);
    b->available += b->w_count;
  uv_mutex_unlock(&b->mutex);
  
  

  return b->w_count / channels;
}