#ifndef _H_DECODER_
#define _H_DECODER_

#include <stdlib.h>
#include <stdint.h>
#include "pcm.h"

typedef struct {
  audio_format_t af;
  void *priv;
} decoder_data_t;

typedef struct {
  char* name;
  char* impl;
  char* ext;
  char* mime;
} decoder_info_t;

typedef struct {
  int (*open)(decoder_data_t *data);
  int (*close)(decoder_data_t *data);

  int (*read)(decoder_data_t *data, uint8_t *buffer, size_t count);
  int (*seek)(decoder_data_t *data, double offset);

  int (*duration)(decoder_data_t *data);
  long (*bitrate)(decoder_data_t *data);
  long (*bitrate_current)(decoder_data_t *data);

  decoder_info_t (*info)(decoder_data_t *data);
} decoder_t;

#endif