#include "printk.h"
#include "vga.h"
#include "portio.h"
#include "ps2.h"

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

  // disable data ports
  while (true) {
    ps2_echo();
  }
}
