#include "interrupts.h"
#include "portio.h"
#include "printk.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

struct IRQTableEntry {
  void *arg;
  irq_handler_t handler;
};

static struct IRQTableEntry irq_table[IDT_MAX_DESCRIPTORS];

void unhandled_irq_handler(int number, int error_code) {
  printk("unhandled interrupt: 0x%x, code: %d\n", number, error_code);
}

void irq_handler(int number, int error_code) {
  CLI;
  if (number < 0 || number >= IDT_MAX_DESCRIPTORS) {
    printk("irq_handler called with invalid interrupt number: %x\n", number);
    STI;
    return;
  }

  struct IRQTableEntry *entry = &irq_table[number];
  if (entry->handler == NULL) {
    unhandled_irq_handler(number, error_code);
  } else {
    entry->handler(number, error_code, entry->arg);
  }

  STI;
}

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

void PIC_remap(int offset) {
  // starts the initialization sequence (in cascade mode)
  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  // ICW2: Master PIC vector offset
  outb(PIC1_DATA, offset);
  // ICW2: Slave PIC vector offset
  outb(PIC2_DATA, offset + 8);
  // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  outb(PIC1_DATA, 4);
  // ICW3: tell Slave PIC its cascade identity (0000 0010)
  outb(PIC2_DATA, 2);

  // ICW4: have the PICs use 8086 mode (and not 8080 mode)
  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);

  // Unmask both PICs.
  outb(PIC1_DATA, 0);
  outb(PIC2_DATA, 0);
}

void IRQ_init() {
  CLI;
  PIC_remap(0x20);
  for (size_t i = 0; i < IDT_MAX_DESCRIPTORS; i += 1) {
    idt_set_descriptor(&idt[i], isr_stub_table[i]);
  }
  idtr.base = (uintptr_t)&idt[0];
  idtr.limit = (uint16_t)sizeof(struct IdtEntry) * IDT_MAX_DESCRIPTORS - 1;

  // clear interrupt table
  memset(irq_table, 0, sizeof(irq_table));

  printk("Initializing interrupts: idt limit: %hu\n", idtr.limit);

  asm volatile("lidt %0" : : "m"(idtr));
  STI;
}

void IRQ_handler_set(int number, irq_handler_t handler, void *arg) {
  // trusting the user (me) doesn't throw a stupid interrupt number at this
  irq_table[number] = (struct IRQTableEntry){
      .handler = handler,
      .arg = arg,
  };
}
