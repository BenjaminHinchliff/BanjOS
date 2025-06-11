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

static size_t pid_count = 0;
static struct ProcessQueue avail_procs = {NULL};
static struct ProcNode source_proc;
static struct ProcContext source_proc_ctx;

struct ProcNode *cur_proc = NULL;
struct ProcNode *next_proc = NULL;

static struct ProcNode *cycle_next_proc(struct ProcessQueue *queue) {
  if (queue->head) {
    struct ProcNode *node = queue->head;
    queue->head = queue->head->next;
    return node;
  } else {
    return NULL;
  }
}

static void unlink_proc(struct ProcNode *node, struct ProcessQueue *queue) {
  if (node != node->prev) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    if (node == queue->head) {
      queue->head = node->next;
    }
  } else {
    queue->head = NULL;
  }
}

static void append_proc(struct ProcNode *node, struct ProcessQueue *queue) {
  if (queue->head) {
    node->prev = queue->head->prev;
    node->next = queue->head;
    node->prev->next = node;
    node->next->prev = node;
  } else {
    node->next = node;
    node->prev = node;
    queue->head = node;
  }
}

void PROC_block_on(struct ProcessQueue *queue, int enable_ints) {
  if (queue == NULL)
    return;

  unlink_proc(cur_proc, &avail_procs);
  append_proc(cur_proc, queue);
  if (enable_ints)
    STI;

  yield();
}

void PROC_unblock_all(struct ProcessQueue *queue) {
  while (queue->head != NULL) {
    PROC_unblock_head(queue);
  }
}

void PROC_unblock_head(struct ProcessQueue *queue) {
  struct ProcNode *node = queue->head;
  assert(node != NULL && "Cannot unblock null head");
  unlink_proc(node, queue);
  append_proc(node, &avail_procs);
}

bool PROC_has_unblocked() { return avail_procs.head != NULL; }

void PROC_init_queue(struct ProcessQueue *queue) { queue->head = NULL; }

void PROC_resume_source() { next_proc = &source_proc; }

void noop_handler(int number, int error_code, void *arg) {}

void kexit_handler(int number, int error_code, void *arg) {
  unlink_proc(cur_proc, &avail_procs);
  kfree(cur_proc->context->frame);
  kfree(cur_proc->context);
  kfree(cur_proc);
  cur_proc = NULL;
  if (avail_procs.head == NULL) {
    // all processes completed
    PROC_resume_source();
  } else {
    // otherwise set next proc to head of proclist
    next_proc = avail_procs.head;
  }
}

void PROC_run() {
  if (avail_procs.head != NULL) {
    IRQ_handler_set(0x80, noop_handler, NULL);
    // set cur_proc to be this thread
    memset(&source_proc_ctx, 0, sizeof(source_proc_ctx));
    memset(&source_proc, 0, sizeof(source_proc));
    source_proc.context = &source_proc_ctx;
    IRQ_handler_set(0x81, kexit_handler, NULL);
    cur_proc = &source_proc;
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
  // frame node
  struct ProcNode *node = kmalloc(sizeof(*node));
  node->context = ctx;
  append_proc(node, &avail_procs);
  return ctx->pid;
}

void PROC_reschedule() { next_proc = cycle_next_proc(&avail_procs); }

void yield() {
  PROC_reschedule();
  asm volatile("int $0x80");
}

void kexit() { asm volatile("int $0x81"); }
