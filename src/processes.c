#include "processes.h"
#include "allocator.h"
#include "gdt.h"
#include "interrupts.h"
#include "smolassert.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PROC_STACK_SIZE 8192

// used to setup new processes
struct InitalProcFrame {
  // stack pointer stored directly in context
  // stack base pointer
  void *rbp;
  // initial saved processor state
  // segment registers
  uint64_t gs, fs, es, ds;
  // standard registers
  uint64_t r15, r14, r13, r12, rbx, r11, r10, r9, r8, rdi, rsi, rdx, rcx, rax;
  // iretq frame, will be set to "return" into entry
  void *rip;
  uint64_t cs;
  uint64_t rflags;
  void *rsp;
  uint64_t ss;
  // kexit return, to be called after the process finishes
  void *kexit_ret;
} __attribute__((packed));

struct ProcNode {
  struct ProcContext *context;
  struct ProcNode *next;
  struct ProcNode *prev;
};

static size_t pid_count = 0;
static struct ProcNode *proclist = {NULL};

struct ProcContext *cur_proc = NULL;
struct ProcContext *next_proc = NULL;

void proclist_queue_proc(struct ProcContext *ctx) {
  struct ProcNode *node = kmalloc(sizeof(*node));
  node->context = ctx;
  if (proclist) {
    node->prev = proclist->prev;
    node->next = proclist;
    node->prev->next = node;
    node->next->prev = node;
  } else {
    node->next = node;
    node->prev = node;
    proclist = node;
  }
}

struct ProcContext *proclist_next_proc() {
  if (proclist) {
    struct ProcContext *ctx = proclist->context;
    proclist = proclist->next;
    return ctx;
  } else {
    return NULL;
  }
}

struct ProcContext *proclist_unqueue_cur_proc() {
  if (proclist) {
    struct ProcNode *head = proclist;
    // remove head from list
    if (head != head->prev) {
      head->prev->next = head->next;
      head->next->prev = head->prev;
      proclist = head->next;
    } else {
      proclist = NULL;
    }
    struct ProcContext *ctx = head->context;
    kfree(head);
    return ctx;
  } else {
    return NULL;
  }
}

void noop_handler(int number, int error_code, void *arg) {}

void kexit_handler(int number, int error_code, void *arg) {
  struct ProcContext *ctx = proclist_unqueue_cur_proc();
  assert(ctx == cur_proc && "must pop correct proc cur_proc!");
  kfree(ctx->frame);
  kfree(ctx);
  cur_proc = NULL;
  if (proclist == NULL) {
    // all processes completed
    next_proc = (struct ProcContext *)arg;
  } else {
    // otherwise set next proc to head of proclist
    next_proc = proclist->context;
  }
}

static inline void *get_rsp() {
  void *rsp;
  asm volatile("mov %%rsp, %0" : "=a"(rsp));
  return rsp;
}

void PROC_run() {
  if (proclist != NULL) {
    IRQ_handler_set(0x80, noop_handler, NULL);
    // set cur_proc to be this thread
    struct ProcContext *orig_ctx = kmalloc(sizeof(*orig_ctx));
    memset(orig_ctx, 0, sizeof(*orig_ctx));
    IRQ_handler_set(0x81, kexit_handler, orig_ctx);
    cur_proc = orig_ctx;
    PROC_reschedule();
    asm volatile("int $0x80");
    IRQ_handler_set(0x80, NULL, NULL);
    IRQ_handler_set(0x81, NULL, NULL);
  }
}

size_t PROC_create_kthread(kproc_t entry_point, void *arg) {
  struct ProcContext *ctx = kmalloc(sizeof(*ctx));
  // make a new stack
  void *frame = kmalloc(PROC_STACK_SIZE);
  struct InitalProcFrame *initial = frame + PROC_STACK_SIZE - sizeof(*initial);
  // zero out frame for good default values for most things
  memset(initial, 0, sizeof(*initial));
  // FIXME: not all threads should be kernel mode in the future
  initial->cs = GDT_kernel_desc_offset();
  // set the function address as the return location for iretq
  initial->rdi = (uint64_t)arg;
  initial->rip = entry_point;
  // after iretq returns the stack should start right at the start
  initial->rsp = frame + PROC_STACK_SIZE - sizeof(void *);
  // add kexit for ret from proc
  initial->kexit_ret = &kexit;
  // set the rsp for the switcher itself
  ctx->frame = (uint64_t *)frame;
  ctx->rsp = (uint64_t *)initial;
  ctx->pid = pid_count++;
  proclist_queue_proc(ctx);
  return ctx->pid;
}

void PROC_reschedule() { next_proc = proclist_next_proc(); }

void yield() {
  PROC_reschedule();
  asm volatile("int $0x80");
}

void kexit() { asm volatile("int $0x81"); }
