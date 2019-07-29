#include <stddef.h>
#include "commandqueue.h"
#include "util/mem.h"

command_queue_t* command_queue_create(const size_t capacity) {
  command_queue_t* q;

  q = mmalloc(offsetof(command_queue_t, data) + capacity*sizeof(command_t));

  q->capacity = capacity;
  q->tail = q->head = q->size = 0;

  uv_mutex_init_recursive(&q->mutex);
  //uv_mutex_init_recursive(&q->poll_mutex);

  uv_cond_init(&q->push_cond);
  uv_cond_init(&q->poll_cond);

  return q;
}

int command_queue_push(command_queue_t* q, command_t command) {
  uv_mutex_lock(&q->mutex);
  if(q->size >= q->capacity) uv_cond_wait(&q->push_cond, &q->mutex);
  
  q->size++;
  uv_mutex_unlock(&q->mutex);

  q->data[q->head] = command;
  q->head = (q->head + 1) % q->capacity;

  uv_cond_signal(&q->poll_cond);

  return 0;
}

int command_queue_poll(command_queue_t* q, command_t* dst) {
  uv_mutex_lock(&q->mutex);
  while(q->size <= 0) uv_cond_wait(&q->poll_cond, &q->mutex);
  q->size--;
  uv_mutex_unlock(&q->mutex);

  command_t command = q->data[q->tail];
  q->tail = (q->tail + 1) % q->capacity;

  uv_cond_signal(&q->push_cond);

  dst->type = command.type;
  dst->arg = command.arg;

  return 0;
}
