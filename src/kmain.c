#include "allocator.h"
#include "block_device.h"
#include "ext2.h"
#include "fs.h"
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
#include "vfs.h"
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
  ps2_initialize();
  while (true) {
    printk("%c", getc());
  }
}

int readdir_printer(const char *filename, struct Inode *inode, void *arg) {
  unsigned long *ino = arg;
  if (strcmp(filename, "fish1.txt") == 0) {
    *ino = inode->ino;
  }
  return 1;
}

void drive_init(void *arg) {
  ext2_init();
  struct BlockDevice *dev = ata_probe(PRIM_IO_BASE, PRIM_CTL_BASE, 0, IRQ14);
  struct SuperBlock *sb = FS_probe(dev);
  printk("sb: %lx\n", sb);
  unsigned long ino;
  sb->root_inode->readdir(sb->root_inode, &readdir_printer, &ino);
  struct Inode *inode = sb->read_inode(sb, ino);
  struct File *file = inode->open(inode);
  char *data = kmalloc(1024);
  int len = file->read(file, data, 1023);
  data[len] = '\0';
  printk("%s\n", data);
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

  /* PROC_create_kthread(&test_thread1, NULL); */
  /* int *fish = kmalloc(sizeof(int)); */
  /* *fish = 420; */
  /* PROC_create_kthread(&test_thread2, &fish); */
  PROC_create_kthread(&keyboard_io, NULL);
  PROC_create_kthread(&drive_init, NULL);

  while (true) {
    PROC_run();
    if (!PROC_has_unblocked()) {
      STI;
      HLT;
    }
  }
}
