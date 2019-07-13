#ifndef _H_DECODER_
#define _H_DECODER_

#include <stdlib.h>
#include <stdint.h>
#include "pcm.h"

typedef struct {
  audio_format_t af;
  void *priv;
} decoder_data_t;

struct decoder_info {
  const char* name;
  const char* impl;

  // This arrays must ends with NULL pointer
  const char *const* ext;
  const char *const* mime;
};

struct decoder {
  int (*open)(decoder_data_t *data);
  int (*close)(decoder_data_t *data);

  int (*read)(decoder_data_t *data, uint8_t *buffer, size_t count);
  int (*seek)(decoder_data_t *data, double offset);

  double (*duration)(decoder_data_t *data);
  long (*bitrate)(decoder_data_t *data);
  long (*bitrate_current)(decoder_data_t *data);
};

#endif