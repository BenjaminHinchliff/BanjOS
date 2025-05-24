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

#define MEMTEST_ADDR_CAPACITY 0x8000
static size_t memtest_addr_len = 0;
static uintptr_t *memtest_addrs[MEMTEST_ADDR_CAPACITY];

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

  for (size_t free = get_free_memory(); free > 0; free = get_free_memory()) {
    uint64_t *page = MMU_alloc_page();
    for (size_t i = 0; i < MMU_PAGE_SIZE / sizeof(*page); ++i) {
      page[i] = (uint64_t)page;
    }
    memtest_addrs[memtest_addr_len++] = page;
  }
  assert(get_free_memory() == 0 && "should allocate all memory");
  for (size_t i = 0; i < memtest_addr_len; ++i) {
    MMU_free_page(memtest_addrs[i]);
  }

  while (true) {
    HLT;
  }
}
