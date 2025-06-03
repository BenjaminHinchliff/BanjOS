#pragma once

#include <stddef.h>
#include <stdint.h>

struct ProcContext {
  // MUST BE FIRST
  uint64_t *rsp;
  size_t pid;
  uint64_t *frame;
} __attribute__((packed));

struct ProcNode {
  struct ProcContext *context;
  struct ProcNode *next;
  struct ProcNode *prev;
};

struct ProcessQueue {
  struct ProcNode *head;
};

extern struct ProcNode *cur_proc;
extern struct ProcNode *next_proc;

void PROC_run();

typedef void (*kproc_t)(void *);
size_t PROC_create_kthread(kproc_t entry_point, void *arg);

void PROC_reschedule();

void PROC_block_on(struct ProcessQueue *, int enable_ints);
void PROC_unblock_all(struct ProcessQueue *);
void PROC_unblock_head(struct ProcessQueue *);
void PROC_init_queue(struct ProcessQueue *);

void yield();

void kexit();
