#include "allocator.h"
#include "block_device.h"
#include "gdt.h"
#include "interrupts.h"
#include "mbr.h"
#include "multiboot_tags.h"
#include "page_allocator.h"
#include "page_table.h"
#include "portio.h"
#include "printk.h"
#include "processes.h"
#include "ps2.h"
#include "serial.h"
#include "smolassert.h" // just macros so clangd thinks it's unused
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

void test_thread1(void *arg) {
  for (size_t i = 0; i < 10; ++i) {
    printk("Thread 1! (%lu - %lx)\n", i, arg);
    yield();
  }
}

void test_thread2(void *arg) {
  for (size_t i = 0; i < 5; ++i) {
    printk("Thread 2! (%lu - %lx)\n", i, arg);
    yield();
  }
}

void keyboard_io(void *arg) {
  while (true) {
    printk("%c", getc());
  }
}

void drive_init(void *arg) {
  struct BlockDevice *dev = ata_probe(PRIM_IO_BASE, PRIM_CTL_BASE, 0, IRQ14);
  BLK_register(dev);
  uint8_t *buf = kmalloc(dev->blk_size);
  dev->read_block(dev, 0, buf);
  struct MBR *mbr = kmalloc(sizeof(*mbr));
  mbr_init(mbr, dev);
  printk("Part LBA: %qu\n", mbr->partitions[0].first_sector_lba);
  printk("Part Size: %qu\n", mbr->partitions[0].num_sectors);
}

void kmain(void) {
  disable_cursor();
  VGA_clear();
  GDT_setup();
  IRQ_init();

  SER_init();

  multiboot_tags_parse_to_mem_regions();

#ifdef MMU_MEMTEST
  MMU_memtest();
#endif

  init_page_table();
  MMU_alloc_init();
  init_alloc();

  ps2_initialize();

  PROC_create_kthread(&test_thread1, NULL);
  int *fish = kmalloc(sizeof(int));
  *fish = 420;
  PROC_create_kthread(&test_thread2, &fish);
  PROC_create_kthread(&keyboard_io, NULL);
  PROC_create_kthread(&drive_init, NULL);

  while (true) {
    PROC_run();
    HLT;
  }
}
