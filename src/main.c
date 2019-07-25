#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "logging.h"
#include "playback.h"
#include "commandqueue.h"
#include "decoder/decoder_impl.h"
#include "pcm.h"
#include "osc_ctrl.h"
#include "util.h"
#include "util_int128.h"

int main() {
  log_init(MIZAR_LOGLEVEL_DEBUG);
  log_write_direct("Mizar (version: %s)", VERSION);

  log_info("Server's pid is %lli", uv_os_getpid());

  playback_init();
  osc_ctrl_init();

  decoder_t decoder = {{0, NULL}, decoder_mp3};
  decoder.ops.open(&decoder.data);  
  
  playback_open(&decoder);

  
  osc_ctrl_start();
  
  return 0;
}
