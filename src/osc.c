#include <stdint.h>
#include <string.h>
#include "osc.h"
#include "util.h"

#define OSC_UINT32_DECODE(b) (((uint32_t)(b)[3]<<0)|((uint32_t)(b)[2]<<8)|((uint32_t)(b)[1]<<16)|((uint32_t)(b)[0]<<24))

int osc_parse_message(osc_message_t* msg, char* buffer, uint32_t len) {
  int i = 0;
  while (buffer[i] != '\0') ++i;
  while (buffer[i] != ',') ++i;
  if (i >= len) return -1; // error: no format string

  msg->format = buffer + i + 1;

  while (i < len && buffer[i] != '\0') ++i;
  if (i == len) return -2; // error: format string in not null terminated

  i = (i + 4) & ~0x3;
  msg->head = buffer + i;

  msg->buffer = buffer;
  msg->size = len;

  return 0;
}

char* osc_get_address(osc_message_t* msg) {
  return msg->buffer; // wow, so simple
}

char* osc_get_format(osc_message_t* msg) {
  return msg->format;
}

uint32_t osc_get_size(osc_message_t* msg) {
  return msg->size;
}

int32_t osc_next_int32(osc_message_t* msg) {
  const int32_t i = OSC_UINT32_DECODE(msg->head);
  msg->head += 4;
  return i;
}

float osc_next_float(osc_message_t* msg) {
  const int32_t i = OSC_UINT32_DECODE(msg->head);
  msg->head += 4;
  return *((float *) (&i));
}

const char* osc_next_string(osc_message_t* msg) {
  int i = (int) strlen(msg->head);
  if (msg->head + i >= msg->buffer + msg->size) return NULL;
  const char *s = msg->head;
  i = (i + 4) & ~0x3;
  msg->head += i;
  return s;
}

void osc_next_blob(osc_message_t* msg, const char **buffer, int *len) {
  int32_t i = OSC_UINT32_DECODE(msg->head);
  if (msg->head + 4 + i <= msg->buffer + msg->size) {
    *len = i;
    *buffer = msg->head + 4;
    i = (i + 7) & ~0x3;
    msg->head += i;
  } else {
    *len = 0;
    *buffer = NULL;
  }
}