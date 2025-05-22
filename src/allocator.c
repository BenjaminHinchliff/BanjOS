#include "allocator.h"
#include "exit.h"
#include "multiboot_tags.h"
#include "printk.h"
#include "smolassert.h"

#include <stdint.h>

struct FreeNode {
  struct FreeNode *next;
};

static struct FreeNode *free_list = NULL;

void *MMU_pf_alloc(void) {
  // allocate from free list first
  if (free_list) {
    void *addr = free_list;
    free_list = free_list->next;
    return addr;
  }

  // otherwise, find next free address in a region
  struct MemRegions *regions = multiboot_get_mem_regions();
  for (size_t i = 0; i < regions->size; ++i) {
    struct MemRegion *region = &regions->d[i];
    if (region->next < region->end) {
      void *addr = region->next;
      region->next += MMU_PAGE_SIZE;
      return addr;
    }
  }

  // out of memory! scream?
  /* printk("ALLOCATOR OUT OF MEMORY!!!"); */
  /* EXIT; */

  // purely for compiler
  return NULL;
}

void MMU_pf_free(void *pf) {
  struct FreeNode *node = (struct FreeNode *)pf;
  node->next = free_list;
  free_list = node;
}

#ifdef MMU_MEMTEST
#define MEMTEST_CYCLES 2
#define MEMTEST_ADDR_CAPACITY 0x8000
static uintptr_t *memtest_addrs[MEMTEST_ADDR_CAPACITY];

void MMU_memtest() {
  // basic allocations
  void *addr1 = MMU_pf_alloc();
  void *addr2 = MMU_pf_alloc();

  assert(addr1 + MMU_PAGE_SIZE == addr2 &&
         "Addresses should be allocated in order");

  MMU_pf_free(addr1);
  MMU_pf_free(addr2);

  void *addr2_new = MMU_pf_alloc();
  void *addr1_new = MMU_pf_alloc();

  assert(addr1 == addr1_new && "Addresses should be reused");
  assert(addr2 == addr2_new && "In reverse order");

  MMU_pf_free(addr1_new);
  MMU_pf_free(addr2_new);

  // count total allocatable blocks
  struct MemRegions *regions = multiboot_get_mem_regions();
  size_t total_pages = 0;
  for (size_t i = 0; i < regions->size; ++i) {
    struct MemRegion *region = &regions->d[i];
    total_pages += (region->end - region->start) / MMU_PAGE_SIZE;
  }
  assert(total_pages <= MEMTEST_ADDR_CAPACITY &&
         "Pages must be less than the capacity of memtest to store addresses");

  // run n cycles of whole memory tests (2 or more allow both allocation
  // mechanisms to be tested)
  for (size_t c = 0; c < MEMTEST_CYCLES; ++c) {
    printk("Running memtest cycle %lu for %lu pages...\n", c, total_pages);
    // allocate and write
    for (size_t i = 0; i < total_pages; ++i) {
      // allocate block
      uintptr_t *page = MMU_pf_alloc();
      // write address to block
      for (size_t j = 0; j < MMU_PAGE_SIZE / sizeof(*page); ++j) {
        page[j] = (uintptr_t)page;
      }
      memtest_addrs[i] = page;
    }
    // verify results
    for (size_t i = 0; i < total_pages; ++i) {
      uintptr_t *page = memtest_addrs[i];
      for (size_t j = 0; j < MMU_PAGE_SIZE / sizeof(*page); ++j) {
        assert(page[j] == (uintptr_t)page &&
               "bit pattern before and after should match!")
      }
      MMU_pf_free(page);
    }
  }

  printk("Memtest passed.\n");
}
#endif
