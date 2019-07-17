#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "commandqueue.h"
#include "decoder/decoder_impl.h"
#include "log.h"
#include "pcm.h"
#include "playback.h"
#include "util.h"

int main() {
  log_init(MIZAR_LOGLEVEL_TRACE);
  log_write_direct("Mizar (version: %s)", VERSION);
  log_info("Server's pid is %lli", uv_os_getpid());

  audio_format_t af = af_endian(0) | af_signed(1) | af_depth(16) |
                      af_rate(44100) | af_channels(2);

  decoder_t decoder = {{af, NULL}, decoder_mp3};
  decoder.ops.open(&decoder.data);

  playback_init();
  
  playback_open(&decoder);
  sec_sleep(1);
  playback_start();
  sec_sleep(1);
  //playback_seek(35);
  sec_sleep(200);
  

  return 0;
}
