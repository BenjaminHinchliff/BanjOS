#pragma once

#include "processes.h"

#include <stdbool.h>

#define BUFF_SIZE 16

struct RingBuffer {
  char buff[BUFF_SIZE];
  char *consumer;
  char *producer;
  struct ProcessQueue *blocked;
};

void ring_init(struct RingBuffer *state, bool blockable);
bool ring_consumer_next(struct RingBuffer *state, char *next);
void ring_consumer_block_next(struct RingBuffer *state, char *next);
bool ring_producer_add_char(struct RingBuffer *state, char to_add);
