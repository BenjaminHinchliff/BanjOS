#include "allocator.h"
#include "gdt.h"
#include "interrupts.h"
#include "multiboot_tags.h"
#include "page_allocator.h"
#include "page_table.h"
#include "portio.h"
#include "printk.h"
#include "processes.h"
#include "ps2.h"
#include "serial.h"
#include "smolassert.h" // just macros so clangd thinks it's unused
#include "snakes.h"
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

#define NUM_ALLOCS 1024

void snakeify(void *arg) { setup_snakes(false); }

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

  init_page_table();
  MMU_alloc_init();
  init_alloc();

  PROC_create_kthread(&snakeify, NULL);

  while (true) {
    PROC_run();
    HLT;
  }
}
