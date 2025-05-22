#include "page_table.h"
#include "allocator.h"
#include "multiboot_tags.h"
#include "printk.h"
#include "smolassert.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
} __attribute__((__packed__));

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
} __attribute__((__packed__));

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
} __attribute__((__packed__));

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
} __attribute__((__packed__));

#define PAGE_LEVEL_SIZE 512

static struct PML4Entry pml4_table[PAGE_LEVEL_SIZE]
    __attribute__((aligned(MMU_PAGE_SIZE)));

void split_virt_addr(void *virt_addr, uint16_t *pml4_off, uint16_t *pdp_off,
                     uint16_t *pd_off, uint16_t *pt_off, uint16_t *off) {
  if (pml4_off) {
    *pml4_off = ((uint64_t)virt_addr >> 39) & 0x1FF;
  }
  if (pdp_off) {
    *pdp_off = ((uint64_t)virt_addr >> 30) & 0x1FF;
  }
  if (pd_off) {
    *pd_off = ((uint64_t)virt_addr >> 21) & 0x1FF;
  }
  if (pt_off) {
    *pt_off = ((uint64_t)virt_addr >> 12) & 0x1FF;
  }
  if (off) {
    *off = (uint64_t)virt_addr & 0xFFF;
  }
}

void *virt_to_phys_addr(void *virt_addr) {
  uint16_t pml4_off, pdp_off, pd_off, pt_off, off;
  split_virt_addr(virt_addr, &pml4_off, &pdp_off, &pd_off, &pt_off, &off);
  struct PDPEntry *pdp_table =
      (struct PDPEntry *)(pml4_table[pml4_off].pdp_addr << 12);
  struct PDEntry *pd_table =
      (struct PDEntry *)(pdp_table[pdp_off].pd_addr << 12);
  struct PTEntry *pt_table = (struct PTEntry *)(pd_table[pd_off].pt_addr << 12);
  return (void *)((pt_table[pt_off].addr << 12) + off);
}

void insert_page(void *virt_addr, void *phys_addr) {
  uint16_t pml4_off, pdp_off, pd_off, pt_off, off;
  split_virt_addr(virt_addr, &pml4_off, &pdp_off, &pd_off, &pt_off, &off);
  struct PML4Entry *pml4_entry = &pml4_table[pml4_off];
  struct PDPEntry *pdp_table;
  if (!pml4_entry->present) {
    pdp_table = MMU_pf_alloc();
    memset(pdp_table, 0, MMU_PAGE_SIZE);
    pml4_entry->present = true;
    pml4_entry->read_write = true;
    pml4_entry->pdp_addr = (uint64_t)pdp_table >> 12;
  } else {
    pdp_table = (struct PDPEntry *)((uint64_t)pml4_entry->pdp_addr << 12);
  }
  struct PDPEntry *pdp_entry = &pdp_table[pdp_off];
  struct PDEntry *pd_table;
  if (!pdp_entry->present) {
    pd_table = MMU_pf_alloc();
    memset(pd_table, 0, MMU_PAGE_SIZE);
    pdp_entry->present = true;
    pdp_entry->read_write = true;
    pdp_entry->pd_addr = (uint64_t)pd_table >> 12;
  } else {
    pd_table = (struct PDEntry *)((uint64_t)pdp_entry->pd_addr << 12);
  }
  struct PDEntry *pd_entry = &pd_table[pd_off];
  struct PTEntry *pt_table;
  if (!pd_entry->present) {
    pt_table = MMU_pf_alloc();
    memset(pt_table, 0, MMU_PAGE_SIZE);
    pd_entry->present = true;
    pd_entry->read_write = true;
    pd_entry->pt_addr = (uint64_t)pt_table >> 12;
  } else {
    pt_table = (struct PTEntry *)((uint64_t)pd_entry->pt_addr << 12);
  }
  struct PTEntry *pt_entry = &pt_table[pt_off];
  pt_entry->present = true;
  pt_entry->read_write = true;
  pt_entry->addr = (uint64_t)phys_addr >> 12;
}

static inline void load_page_table(struct PML4Entry *pml4_table) {
  asm volatile("mov %0, %%cr3" : : "r"(pml4_table));
}

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
 *
 */
void init_page_table() {
  assert(sizeof(pml4_table) == MMU_PAGE_SIZE);
  memset(pml4_table, 0, sizeof(pml4_table));
  struct MemRegions *regions = multiboot_get_mem_regions();
  void *mem_end = regions->d[regions->size - 1].end;
  void *addr = 0x0;
  while (addr < mem_end) {
    insert_page(addr, addr);
    addr += MMU_PAGE_SIZE;
  }
  uint16_t pml4_off, pdp_off, pd_off, pt_off, off;
  split_virt_addr(pml4_table, &pml4_off, &pdp_off, &pd_off, &pt_off, &off);
  printk("address components: %lx -> [PML %hx][PDP %hx][PD %hx][PT %hx][OFF "
         "%hx]\n",
         (uintptr_t)pml4_table, pml4_off, pdp_off, pd_off, pt_off, off);
  printk("translation test: %lx -> %lx\n", (uintptr_t)pml4_table,
         (uintptr_t)virt_to_phys_addr(pml4_table));
  load_page_table(pml4_table);
}
