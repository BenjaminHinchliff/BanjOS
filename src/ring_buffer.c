#include "ring_buffer.h"

#include "allocator.h"
#include "interrupts.h"
#include "processes.h"

#include <stdbool.h>

static struct RingBuffer ring;

void ring_init(struct RingBuffer *state, bool blockable) {
  state->consumer = &state->buff[0];
  state->producer = &state->buff[0];
  if (blockable) {
    state->blocked = kmalloc(sizeof((*state->blocked)));
    PROC_init_queue(state->blocked);
  } else {
    state->blocked = NULL;
  }
}

bool ring_consumer_next(struct RingBuffer *state, char *next) {
  if (state->consumer == state->producer) {
    return false;
  }

  *next = *state->consumer++;

  if (state->consumer >= &state->buff[BUFF_SIZE]) {
    state->consumer = &state->buff[0];
  }
  return true;
}

void ring_consumer_block_next(struct RingBuffer *state, char *next) {
  CLI;
  while (state->consumer == state->producer) {
    PROC_block_on(state->blocked, true);
    CLI;
  }

  *next = *state->consumer++;

  if (state->consumer >= &state->buff[BUFF_SIZE]) {
    state->consumer = &state->buff[0];
  }
  STI;
}

bool ring_producer_add_char(struct RingBuffer *state, char to_add) {
  if (state->producer == state->consumer - 1 ||
      (state->consumer == &state->buff[0] &&
       state->producer == &state->buff[BUFF_SIZE - 1])) {
    return false;
  }

  *state->producer++ = to_add;

  if (state->producer >= &state->buff[BUFF_SIZE]) {
    state->producer = &state->buff[0];
  }
  return true;
}
