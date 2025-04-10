#include "printk.h"
#include "vga.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void disable_cursor(void) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}

void kmain(void) {
  // Print OKAY
  disable_cursor();
  VGA_clear();

  // printk tests
  int n = printk("");
  printk("%d\n", n);                   // should be zero
  n = printk("%c\n", 'a');             // should be "a"
  printk("%d\n", n);                   // should be 2
  printk("%c\n", 'Q');                 // should be "Q"
  printk("%c\n", 256 + '9');           // Should be "9"
  printk("%s\n", "test string");       // "test string"
  printk("foo%sbar\n", "blah");        // "fooblahbar"
  printk("foo%%sbar\n");               // "foo%bar"
  printk("%d\n", INT_MIN);             // "-2147483648"
  printk("%d\n", INT_MAX);             // "2147483647"
  printk("%u\n", 0);                   // "0"
  printk("%u\n", UINT_MAX);            // "4294967295"
  printk("%x\n", 0xDEADbeef);          // "deadbeef"
  printk("%p\n", (void *)UINTPTR_MAX); // "0xFFFFFFFFFFFFFFFF"
  printk("%hd\n", 0x8000);             // "-32768"
  printk("%hd\n", 0x7FFF);             // "32767"
  printk("%hu\n", 0xFFFF);             // "65535"
  printk("%ld\n", LONG_MIN);           // "-9223372036854775808"
  printk("%ld\n", LONG_MAX);           // "9223372036854775807"
  printk("%lu\n", ULONG_MAX);          // "18446744073709551615"
  printk("%qd\n", LLONG_MIN);          // "-9223372036854775808"
  printk("%qd\n", LLONG_MAX);          // "9223372036854775807"
  printk("%qu\n", ULLONG_MAX);         // "18446744073709551615"

  while (true) {
    asm volatile("hlt");
  }
}
