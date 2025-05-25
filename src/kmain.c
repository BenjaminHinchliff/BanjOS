#include "allocator.h"
#include "exit.h"
#include "gdt.h"
#include "interrupts.h"
#include "keycodes.h"
#include "multiboot_tags.h"
#include "page_allocator.h"
#include "page_table.h"
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

#define NUM_ALLOCS 1024

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

  uint64_t **fishes = kmalloc(sizeof(*fishes) * NUM_ALLOCS);

  for (size_t i = 0; i < NUM_ALLOCS; ++i) {
    uint64_t *fish = kmalloc(sizeof(*fish));
    *fish = (uint64_t)fish;
    fishes[i] = fish;
  }
  for (size_t i = 0; i < NUM_ALLOCS; ++i) {
    kfree(fishes[i]);
  }
  kfree(fishes);

  printk("test complete\n");

  while (true) {
    HLT;
  }
}
