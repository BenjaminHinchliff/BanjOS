#include "allocator.h"
#include "page_allocator.h"
#include "smolassert.h"

#include <stddef.h>
#include <stdint.h>

struct FreeList {
  struct FreeList *next;
};

struct KmallocPool {
  size_t max_size;
  struct FreeList *head;
};

void pool_allocate_page(struct KmallocPool *pool) {
  // FIXME: needs to be finished
  void *page = NULL; // sbrk(MMU_PAGE_SIZE);
  assert(MMU_PAGE_SIZE % pool->max_size == 0 &&
         "pool size must be a multiple of block size");
  for (void *block = page; block < page + MMU_PAGE_SIZE;
       block += pool->max_size) {
    struct FreeList *free_node = (struct FreeList *)block;
    free_node->next = pool->head;
    pool->head = free_node;
  }
}

struct KmallocHeader {
  struct KmallocPool *pool;
  size_t size;
};

size_t block_sizes[] = {32, 64, 128, 512, 1024, 2048, 4096};
#define BLOCK_SIZES_LEN (sizeof(block_sizes) / sizeof(*block_sizes))

static struct KmallocPool pools[BLOCK_SIZES_LEN];

void init_alloc() {
  for (size_t i = 0; i < BLOCK_SIZES_LEN; ++i) {
    struct KmallocPool *pool = &pools[i];
    pool->max_size = block_sizes[i] - sizeof(struct KmallocHeader);
    pool->head = NULL;
  }
}

void *kmalloc(size_t size) {
  // find pool
  struct KmallocPool *pool = pools;
  for (; size > pool->max_size; ++pool)
    ;
  if (pool->head == NULL) {
    pool_allocate_page(pool);
  }
  void *addr = pool->head;
  pool->head = pool->head->next;
  return addr;
}

void kfree(void *addr) {}
