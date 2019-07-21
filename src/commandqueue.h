#ifndef _H_COMMAND_QUEUE_
#define _H_COMMAND_QUEUE_

#include <stdint.h>
#include <uv.h>

typedef struct {
  char type;
  union {
    int i;
    float f;
    long l;
    double d;
    void* ptr;
  } arg;
} command_t;

typedef struct {
  size_t capacity;

  volatile size_t head;
  volatile size_t tail;
  volatile size_t size;

  uv_mutex_t push_mutex;
  uv_mutex_t poll_mutex;

  uv_cond_t push_cond;
  uv_cond_t poll_cond;

  command_t data[];
} command_queue_t;

command_queue_t* command_queue_create(const size_t capacity);

int command_queue_push(command_queue_t* queue, command_t command);
int command_queue_poll(command_queue_t* queue, command_t* dst);

#endif