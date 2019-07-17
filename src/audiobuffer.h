#ifndef _H_AUDIOBUFFER_
#define _H_AUDIOBUFFER_

#include <stdint.h>
#include <uv.h>
#include "pcm.h"

typedef struct {
  audio_format_t af;
  double frames;
  size_t capacity;
  size_t size;

  volatile size_t available;
  volatile size_t read_index;
  volatile size_t write_index;
  
  double time;

  uv_mutex_t mutex;

  float data[];
} audiobuffer_t;

audiobuffer_t* audiobuffer_create(audio_format_t af, size_t capacity);

void audiobuffer_destroy(audiobuffer_t* b);
void audiobuffer_reset(audiobuffer_t* b);
size_t audiobuffer_get_available(audiobuffer_t* b);
size_t audiobuffer_get_space(audiobuffer_t* b);
double audiobuffer_get_time(audiobuffer_t* b);
void audiobuffer_set_time(audiobuffer_t* b, double time);
size_t audiobuffer_read(audiobuffer_t* b, float* dst, const size_t len);
size_t audiobuffer_consume(audiobuffer_t* b, const size_t count);
size_t audiobuffer_write(audiobuffer_t* b, float* src, const size_t len);
size_t audiobuffer_fill(audiobuffer_t* b, const size_t count);

#endif