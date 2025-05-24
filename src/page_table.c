#include "page_table.h"
#include "multiboot_tags.h"
#include "page_allocator.h"
#include "printk.h"
#include "smolassert.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PAGE_TABLE_LEVEL_SIZE 512
#define PAGE_TABLE_NUM_LEVELS 4

void split_virt_addr(void *virt_addr, uint16_t indices[PAGE_TABLE_NUM_LEVELS],
                     uint16_t *offset) {
  if (indices) {
    for (int l = 0; l < PAGE_TABLE_NUM_LEVELS; ++l) {
      indices[l] = ((uint64_t)virt_addr >> (9 * l + 12)) & 0x1FF;
    }
  }
  // page offset
  if (offset) {
    *offset = (uint64_t)virt_addr & 0xFFF;
  }
}

struct PTEntry *page_table_get_entry(struct PageEntry *table, void *virt_addr,
                                     bool allocate) {
  uint16_t indices[PAGE_TABLE_NUM_LEVELS], offset;
  split_virt_addr(virt_addr, indices, &offset);
  for (int l = PAGE_TABLE_NUM_LEVELS - 1; l > 0; --l) {
    struct PageEntry *entry = &table[indices[l]];
    if (!entry->present) {
      if (allocate) {
        struct PageEntry *next_table = MMU_pf_alloc();
        memset(next_table, 0, MMU_PAGE_SIZE);
        entry->present = true;
        entry->read_write = true;
        entry->addr = (uint64_t)next_table >> 12;
      } else {
        return NULL;
      }
    }
    table = (struct PageEntry *)((uint64_t)entry->addr << 12);
  }
  return (struct PTEntry *)&table[indices[0]];
}

void *page_table_virt_to_phys_addr(struct PageEntry *table, void *virt_addr) {
  struct PTEntry *entry = page_table_get_entry(table, virt_addr, false);
  if (entry == NULL) {
    return NULL;
  }
  uint16_t offset;
  split_virt_addr(virt_addr, NULL, &offset);
  return (void *)(((uint64_t)entry->addr << 12) + offset);
}

typedef void (*entry_callback)(void *addr, struct PTEntry *entry);

void direct_map_callback(void *addr, struct PTEntry *entry) {
  entry->present = true;
  entry->read_write = true;
  entry->addr = (uint64_t)addr >> 12;
}

void page_table_walk(struct PageEntry *table, void *start_addr, void *end_addr,
                     entry_callback callback) {
  uint16_t indices[4], offset;
  for (void *addr = start_addr; addr < end_addr; addr += MMU_PAGE_SIZE) {
    struct PTEntry *entry = page_table_get_entry(table, addr, true);
    callback(addr, entry);
  }
}

static inline void load_page_table(struct PML4Entry *pml4_table) {
  asm volatile("mov %0, %%cr3" : : "r"(pml4_table));
}

struct PML4Entry *init_page_table() {
  struct PML4Entry *pml4_table = MMU_pf_alloc();
  memset(pml4_table, 0, MMU_PAGE_SIZE);
  struct MemRegions *regions = multiboot_get_mem_regions();
  void *mem_end = regions->d[regions->size - 1].end;
  page_table_walk((struct PageEntry *)pml4_table, (void *)MMU_PAGE_SIZE,
                  mem_end, &direct_map_callback);
  uint16_t indices[4], offset;
  split_virt_addr((void *)(2lu << 39), indices, &offset);
  printk("address components: %lx -> [PML %hx][PDP %hx][PD %hx][PT %hx][OFF "
         "%hx]\n",
         (uintptr_t)(2lu << 39), indices[3], indices[2], indices[1], indices[0],
         offset);
  printk("translation test: %lx -> %lx\n", (uintptr_t)0x69420,
         (uintptr_t)page_table_virt_to_phys_addr((struct PageEntry *)pml4_table,
                                                 (void *)0x69420));
  load_page_table(pml4_table);
  return pml4_table;
}
