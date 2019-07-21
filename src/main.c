#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "logging.h"
#include "playback.h"
#include "commandqueue.h"
#include "decoder/decoder_impl.h"
#include "pcm.h"
#include "util.h"
#include "util_int128.h"

int main() {
  log_init(MIZAR_LOGLEVEL_DEBUG);
  log_write_direct("Mizar (version: %s)", VERSION);

  log_info("Server's pid is %lli", uv_os_getpid());

  decoder_t decoder = {{0, NULL}, decoder_mp3};
  decoder.ops.open(&decoder.data);

  playback_init();
  
  playback_open(&decoder);
  sec_sleep(1);
  playback_start();
  sec_sleep(1);
  playback_seek(35 * 1000);
  sec_sleep(200);
  
  return 0;
}
