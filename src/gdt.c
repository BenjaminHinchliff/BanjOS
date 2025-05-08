#include "gdt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DESCRIPTOR_FLAG_CONFORMING (1lu << 42)
#define DESCRIPTOR_FLAG_EXECUTABLE (1lu << 43)
#define DESCRIPTOR_FLAG_USER_SEGMENT (1lu << 44)
#define DESCRIPTOR_FLAG_PRESENT (1lu << 47)
#define DESCRIPTOR_FLAG_LONG_MODE (1lu << 53)

#define GDT_SIZE 8
static struct {
  uint64_t table[GDT_SIZE];
  size_t top;
} gdt = {.table = {}, .top = 0};

struct Gdtr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));
static struct Gdtr gdtr;

static size_t kernel_desc_offset = 0;

static inline void lgdt(const struct Gdtr *addr) {
  asm volatile("lgdt (%0)" : : "r"(addr));
}

size_t GDT_push_dq(uint64_t data) {
  gdt.table[gdt.top] = data;
  return gdt.top++ * sizeof(uint64_t);
}

size_t GDT_push(uint64_t *data, size_t len) {
  size_t location = gdt.top * sizeof(uint64_t);
  for (size_t i = 0; i < len; i += 1) {
    GDT_push_dq(data[i]);
  }
  return location;
}

void GDT_load() {
  gdtr.limit = sizeof(uint64_t) * gdt.top - 1;
  gdtr.base = (uint64_t)gdt.table;
  lgdt(&gdtr);
}

size_t GDT_kernel_desc_offset() { return kernel_desc_offset; }

void GDT_setup() {
  GDT_push_dq(0);
  kernel_desc_offset =
      GDT_push_dq(DESCRIPTOR_FLAG_USER_SEGMENT | DESCRIPTOR_FLAG_PRESENT |
                  DESCRIPTOR_FLAG_EXECUTABLE | DESCRIPTOR_FLAG_LONG_MODE);
  GDT_load();
}
