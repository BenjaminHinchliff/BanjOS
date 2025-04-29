#include "printk.h"
#include "vga.h"
#include "portio.h"
#include "ps2.h"
#include "interrupts.h"

#include <keycodes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void disable_cursor(void) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}

void kmain(void) {
  // Print OKAY
  disable_cursor();
  VGA_clear();
  ps2_initialize();
  IRQ_init();

  asm volatile("int $0x20");
  printk("didn't fuck anything\n");


  while (true) {
    CLI;
    asm volatile("hlt");
  }
}
