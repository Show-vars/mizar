#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "playback.h"
#include "util.h"

int main() {
  log_init();
  log_write_direct("Mizar (version: %s)", VERSION);

  playback_init();

  return 0;
}
