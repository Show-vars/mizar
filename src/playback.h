#ifndef _H_PLAYBACK_
#define _H_PLAYBACK_

#include "commandqueue.h"
#include "decoder/decoder.h"

#define PLAYBACK_CMD_NOTHING  0
#define PLAYBACK_CMD_OPEN     1
#define PLAYBACK_CMD_CLOSE    2
#define PLAYBACK_CMD_START    3
#define PLAYBACK_CMD_STOP     4
#define PLAYBACK_CMD_SEEK     5
#define PLAYBACK_CMD_SHUTDOWN 6

typedef struct {
  double time;
  double rms;
  double peak;
} playback_realtime_data_t;

void playback_init(void);
int playback_ctl(command_t command);
int playback_open(decoder_t* decoder);
int playback_close();
int playback_start();
int playback_stop();
int playback_seek(double offset);
int playback_shutdown();

#endif