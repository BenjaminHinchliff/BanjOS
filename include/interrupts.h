#pragma once

#include <portio.h>
#include <stdbool.h>
#include <stdint.h>

#define CLI asm volatile("cli")
#define STI asm volatile("sti")

#define FLAGS_IF (0x0200)

static inline bool are_interrupts_enabled() {
  uint64_t flags;
  asm volatile("pushfq; popq %0" : "=r"(flags));
  return (flags & FLAGS_IF) != 0;
}

#define CLI_GUARD                                                              \
  bool enable_ints = false;                                                    \
  if (are_interrupts_enabled()) {                                              \
    enable_ints = true;                                                        \
    CLI;                                                                       \
  }

#define STI_GUARD                                                              \
  if (enable_ints) {                                                           \
    STI;                                                                       \
  }

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20 /* End-of-interrupt command code */

static inline void PIC_sendEOI(uint8_t irq) {
  if (irq >= 8)
    outb(PIC2_COMMAND, PIC_EOI);

  outb(PIC1_COMMAND, PIC_EOI);
}

typedef void (*irq_handler_t)(int number, int error_code, void *arg);

#define IRQ_BASE 0x20
enum IRQLine {
  IRQ0 = 0,
  IRQ1 = 1,
  IRQ2 = 2,
  IRQ3 = 3,
  IRQ4 = 4,
  IRQ5 = 5,
  IRQ6 = 6,
  IRQ7 = 7,
  IRQ8 = 8,
  IRQ9 = 9,
  IRQ10 = 10,
  IRQ11 = 11,
  IRQ12 = 12,
  IRQ13 = 13,
  IRQ14 = 14,
  IRQ15 = 15,
};

void IRQ_init();
void IRQ_handler_set(int number, irq_handler_t handler, void *arg);
void IRQ_set_mask(uint8_t IRQline);
void IRQ_clear_mask(uint8_t IRQline);
