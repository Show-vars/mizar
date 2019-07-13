#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "log.h"
#include "playback.h"
#include "commandqueue.h"
#include "decoder/decoder_impl.h"
#include "pcm.h"
#include "util.h"

int main() {
  log_init(MIZAR_LOGLEVEL_DEBUG);
  log_write_direct("Mizar (version: %s)", VERSION);

  log_info("Server's pid is %lli", uv_os_getpid());

  playback_init();

  audio_format_t af = af_endian(0) | af_signed(1) | af_depth(16) |
                      af_rate(44100) | af_channels(2);

  decoder_t decoder = { {af, NULL}, decoder_mp3 };
  decoder.ops.open(&decoder.data);

  double offset = 36;

  command_t command_open = { PLAYBACK_CMD_OPEN, &decoder };
  command_t command_start = { PLAYBACK_CMD_START, NULL };
  command_t command_seek = { PLAYBACK_CMD_SEEK, &offset };

  playback_ctl(command_open);
  playback_ctl(command_start);

  msleep(3 * 1000);
  
  playback_ctl(command_seek);

  msleep(200 * 1000);

  return 0;
}
