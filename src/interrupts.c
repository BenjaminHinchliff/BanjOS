#include "printk.h"
#include "interrupts.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// TODO: find a way to not have to hardcod e this
#define GDT_OFFSET_KERNEL_CODE (1 << 3)
#define IDT_INTERRUPT_TYPE_INTERRUPT_GATE 0xE

struct IdtEntry {
  uint16_t isr_low;
  uint16_t kernel_cs;
  uint8_t ist : 3;
  uint32_t reserved1 : 5;
  uint8_t type : 4;
  bool zero : 1;
  uint8_t dpl : 2;
  bool present : 1;
  uint16_t isr_mid;
  uint32_t isr_high;
  uint32_t reserved2;
} __attribute__((packed));

struct Idtr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

#define IDT_MAX_DESCRIPTORS 256

__attribute__((aligned(0x10))) static struct IdtEntry idt[IDT_MAX_DESCRIPTORS];
static struct Idtr idtr;

static void idt_set_descriptor(struct IdtEntry *descriptor, void *isr) {
  descriptor->isr_low = (uint64_t)isr & 0xFFFF;
  descriptor->kernel_cs = GDT_OFFSET_KERNEL_CODE;
  descriptor->ist = 0;
  descriptor->reserved1 = 0;
  descriptor->type = IDT_INTERRUPT_TYPE_INTERRUPT_GATE;
  descriptor->zero = 0;
  descriptor->dpl = 0;
  descriptor->present = 1;
  descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
  descriptor->reserved2 = 0;
}

extern void *isr_stub_table[];

void irq_handler(int number, int error_code) {
  CLI;
  printk("handling interrupt: %d, code: %d\n", number, error_code);
  STI;
}

void IRQ_init() {
  CLI;
  for (size_t i = 0; i < IDT_MAX_DESCRIPTORS; i += 1) {
    idt_set_descriptor(&idt[i], isr_stub_table[i]);
  }
  idtr.base = (uintptr_t)&idt[0];
  idtr.limit = (uint16_t)sizeof(struct IdtEntry) * IDT_MAX_DESCRIPTORS - 1;
  printk("Initializing interrupts: idt limit: %hu\n", idtr.limit);

  asm volatile("lidt %0" : : "m"(idtr));
  STI;
}
