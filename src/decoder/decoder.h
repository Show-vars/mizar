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
  const char* name;
  const char* impl;

  // This arrays must ends with NULL pointer
  const char *const* ext;
  const char *const* mime;
} decoder_info_t;

typedef struct {
  int (*open)(decoder_data_t *data);
  int (*close)(decoder_data_t *data);

  size_t (*read_s16)(decoder_data_t *data, uint8_t *buffer, size_t frames);
  int (*seek)(decoder_data_t *data, long offset);

  long (*duration)(decoder_data_t *data);
  long (*bitrate)(decoder_data_t *data);
  long (*bitrate_current)(decoder_data_t *data);
} decoder_ops_t;

typedef struct {
  decoder_data_t data;
  decoder_ops_t ops;
} decoder_t;



#define decoder_set(dst,src)  ((decoder_ops_t*)(dst))->open            = ((decoder_ops_t*)(src))->open,\
                              ((decoder_ops_t*)(dst))->close           = ((decoder_ops_t*)(src))->close,\
                              ((decoder_ops_t*)(dst))->read_s16        = ((decoder_ops_t*)(src))->read_s16,\
                              ((decoder_ops_t*)(dst))->seek            = ((decoder_ops_t*)(src))->seek,\
                              ((decoder_ops_t*)(dst))->duration        = ((decoder_ops_t*)(src))->duration,\
                              ((decoder_ops_t*)(dst))->bitrate         = ((decoder_ops_t*)(src))->bitrate,\
                              ((decoder_ops_t*)(dst))->bitrate_current = ((decoder_ops_t*)(src))->bitrate_current;

#endif