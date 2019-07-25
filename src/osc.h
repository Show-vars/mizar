#ifndef _H_OSCM_
#define _H_OSCM_

#include <stdint.h>

typedef struct {
  uint32_t size;
  char *format;

  char *head;
  char *buffer;
} osc_message_t;

int osc_parse_message(osc_message_t* msg, char* buffer, uint32_t len);
char* osc_get_address(osc_message_t* msg);
char* osc_get_format(osc_message_t* msg);
uint32_t osc_get_size(osc_message_t* msg);
int32_t osc_next_int32(osc_message_t* msg);
float osc_next_float(osc_message_t* msg);
const char* osc_next_string(osc_message_t* msg);
void osc_next_blob(osc_message_t* msg, const char **buffer, int *len);

#endif