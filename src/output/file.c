#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "output.h"
#include "pcm.h"
#include "util.h"

FILE* f;

static int output_file_init() {
  f = fopen("./output.wav", "wb");

  return 0;
}

static int output_file_destroy() {
  fclose(f);

  return 0;
}

static int output_file_open(audio_format_t af) {
  return 0;
}

static int output_file_close() {
  return 0;
}

static int output_file_write(const uint8_t *buf, size_t len) {
  fwrite(buf, 1, len, f);

  return 0;
}

static int output_file_buffer_space() {
  return 1024*1024;
}

static int output_file_pause() {
  return 0;
}

static int output_file_unpause() {
  return 0;
}

const struct output_device output_device_ops = {
    .init = output_file_init,
    .destroy = output_file_destroy,
    .open = output_file_open,
    .close = output_file_close,
    .write = output_file_write,
    .buffer_space = output_file_buffer_space,
    .pause = output_file_pause,
    .unpause = output_file_unpause,
};