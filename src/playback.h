#ifndef _H_PLAYBACK_
#define _H_PLAYBACK_

#include "commandqueue.h"

#define PLAYBACK_CMD_NOTHING  0
#define PLAYBACK_CMD_OPEN     1
#define PLAYBACK_CMD_CLOSE    2
#define PLAYBACK_CMD_START    3
#define PLAYBACK_CMD_STOP     4
#define PLAYBACK_CMD_SEEK     5
#define PLAYBACK_CMD_SHUTDOWN 6

void playback_init(void);
int playback_ctl(command_t command);

#endif