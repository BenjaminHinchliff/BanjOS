#include "allocator.h"
#include "exit.h"
#include "gdt.h"
#include "interrupts.h"
#include "keycodes.h"
#include "multiboot_tags.h"
#include "portio.h"
#include "printk.h"
#include "ps2.h"
#include "serial.h"
#include "smolassert.h"
#include "vga.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void disable_cursor(void) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}

void kmain(void) {
  disable_cursor();
  VGA_clear();
  GDT_setup();
  IRQ_init();

  SER_init();
  ps2_initialize();

  multiboot_tags_parse_to_mem_regions();

#ifdef MMU_MEMTEST
  MMU_memtest();
#endif

  while (true) {
    HLT;
  }
}
