#include "interrupts.h"
#include "portio.h"
#include "ps2.h"
#include "vga.h"

#include <keycodes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void disable_cursor(void) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}


void PIT_handler(int num, int error_code, void *arg) { PIC_sendEOI(num); }

void kmain(void) {
  disable_cursor();
  VGA_clear();
  IRQ_init();
  ps2_initialize();

  // set up a no-op handler for the programmable interrupt timer so it doesn't
  // clog up the PIC lines
  IRQ_handler_set(0x20, PIT_handler, NULL);

  while (true) {
  }
}
