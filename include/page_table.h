#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ADDR_SPACE_PHYSICAL_PAGE_BASE (void *)0x0
#define ADDR_SPACE_KERNEL_HEAP_BASE (void *)(0x1lu << 39)
#define ADDR_SPACE_RESERVED_GROWTH_BASE (void *)(0x2lu << 39)
#define ADDR_SPACE_KERNEL_STACKS_BASE (void *)(0xFlu << 39)
#define ADDR_SPACE_USER_SPACE_BASE (void *)(0x10lu << 39)

struct PML4Entry {
  bool present : 1;
  bool read_write : 1;
  bool user_supervisor : 1;
  bool page_write_through : 1;
  bool page_cache_disable : 1;
  bool accessed : 1;
  bool ignored : 1;
  uint8_t must_be_zero : 2;
  uint8_t available1 : 3;
  uint64_t pdp_addr : 40;
  uint16_t available2 : 11;
  bool no_execute : 1;
} __attribute__((packed));

struct PDPEntry {
  bool present : 1;
  bool read_write : 1;
  bool user_supervisor : 1;
  bool page_write_through : 1;
  bool page_cache_disable : 1;
  bool accessed : 1;
  bool ignored : 1;
  uint8_t must_be_zero : 2;
  uint8_t available1 : 3;
  uint64_t pd_addr : 40;
  uint16_t available2 : 11;
  bool no_execute : 1;
} __attribute__((packed));

struct PDEntry {
  bool present : 1;
  bool read_write : 1;
  bool user_supervisor : 1;
  bool page_write_through : 1;
  bool page_cache_disable : 1;
  bool accessed : 1;
  bool ignored1 : 1;
  uint8_t must_be_zero : 1;
  bool ignored2 : 1;
  uint8_t available1 : 3;
  uint64_t pt_addr : 40;
  uint16_t available2 : 11;
  bool no_execute : 1;
} __attribute__((packed));

struct PTEntry {
  bool present : 1;
  bool read_write : 1;
  bool user_supervisor : 1;
  bool page_write_through : 1;
  bool page_cache_disable : 1;
  bool accessed : 1;
  bool dirty : 1;
  bool page_attribute_table : 1;
  bool global : 1;
  uint8_t available1 : 3;
  uint64_t addr : 40;
  uint16_t available2 : 11;
  bool no_execute : 1;
} __attribute__((packed));

struct PageEntry {
  bool present : 1;
  bool read_write : 1;
  bool user_supervisor : 1;
  uint8_t : 6;
  uint8_t available : 3;
  uint64_t addr : 40;
  uint16_t : 12;
} __attribute__((packed));

/**
 * address layout:
 * +----------------+---------------------+
 * | 0x000000000000 | physical page frames|
 * +----------------+---------------------+
 * | 0x010000000000 | kernel heap         |
 * +----------------+---------------------+
 * | 0x020000000000 | reserved/growth     |
 * +----------------+---------------------+
 * | 0x0F0000000000 | kernel stacks       |
 * +----------------+---------------------+
 * | 0x100000000000 | user space          |
 * +----------------+---------------------+
 */
struct PML4Entry *init_page_table();

static inline struct PML4Entry *get_current_page_table() {
  struct PML4Entry *table;
  asm volatile("mov %%cr3, %0" : "=a"(table));
  return table;
}

static inline void *get_cr2() {
  void *cr2;
  asm volatile("mov %%cr2, %0" : "=a"(cr2));
  return cr2;
}

struct PTEntry *page_table_get_entry(struct PageEntry *table, void *virt_addr,
                                     bool allocate);

typedef void (*entry_callback)(void *addr, struct PTEntry *entry);
void page_table_walk(struct PageEntry *table, void *start_addr, void *end_addr,
                     entry_callback callback);
