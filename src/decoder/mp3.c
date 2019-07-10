#include <stdlib.h>
#include <stdint.h>
#include "pcm.h"
#include "decoder/decoder_impl.h"

int decoder_mp3_open(decoder_data_t *data) {
  return -1;
}

int decoder_mp3_close(decoder_data_t *data) {
  return -1;
}

int decoder_mp3_read(decoder_data_t *data, uint8_t *buffer, size_t count) {
  return -1;
}

int decoder_mp3_seek(decoder_data_t *data, double offset) {
  return -1;
}

int decoder_mp3_duration(decoder_data_t *data) {
  return -1;
}

long decoder_mp3_bitrate(decoder_data_t *data) {
  return 0;
}


long decoder_mp3_bitrate_current(decoder_data_t *data) {
  return 0;
}

static char* DECODER_NAME = "mp3";  
static char* DECODER_IMPL = "dr_mp3";  
static char* DECODER_EXT = "mp3";  
static char* DECODER_MIME = "audio/mpeg";  

decoder_info_t decoder_mp3_info(decoder_data_t *data) {
  decoder_info_t decoder_info = {
    .name = DECODER_NAME,
    .impl = DECODER_IMPL,
    .ext = DECODER_EXT,
    .mime = DECODER_MIME
  };

  return decoder_info;
}

decoder_t decoder_impl_mp3() {
  decoder_t decoder = {
    .open = decoder_mp3_open,
    .close = decoder_mp3_close,

    .read = decoder_mp3_read,
    .seek = decoder_mp3_seek,

    .duration = decoder_mp3_duration,
    .bitrate = decoder_mp3_bitrate,
    .bitrate_current = decoder_mp3_bitrate_current,
    .info = decoder_mp3_info
  };

  return decoder;
}