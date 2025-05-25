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

struct KmallocHeader {
  struct KmallocPool *pool;
  size_t size;
} __attribute__((packed));

static void pool_allocate_page(struct KmallocPool *pool) {
  void *page = MMU_alloc_page();
  assert(MMU_PAGE_SIZE % (pool->max_size + sizeof(struct KmallocHeader)) == 0 &&
         "pool size must be a multiple of block size");
  for (void *block = page; block < page + MMU_PAGE_SIZE;
       block += pool->max_size + sizeof(struct KmallocHeader)) {
    struct FreeList *free_node = (struct FreeList *)block;
    free_node->next = pool->head;
    pool->head = free_node;
  }
}

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
  size_t i;
  struct KmallocPool *pool;
  for (i = 0, pool = pools; i < BLOCK_SIZES_LEN && size > pool->max_size;
       ++i, ++pool)
    ;
  // one of the pools is large enough, otherwise dedicated allocation
  if (i < BLOCK_SIZES_LEN) {
    if (pool->head == NULL) {
      pool_allocate_page(pool);
    }
    void *addr = pool->head;
    pool->head = pool->head->next;
    struct KmallocHeader *header = addr;
    header->pool = pool;
    header->size = 0;
    return addr + sizeof(struct KmallocHeader);
  } else {
    size += sizeof(struct KmallocHeader);
    size_t num_pages = size / MMU_PAGE_SIZE + !!(size % MMU_PAGE_SIZE);
    void *addr = MMU_alloc_pages(num_pages);
    struct KmallocHeader *header = addr;
    header->pool = NULL;
    header->size = size;
    return addr + sizeof(struct KmallocHeader);
  }
}

void kfree(void *addr) {
  void *block = (addr - sizeof(struct KmallocHeader));
  struct KmallocHeader header = *(struct KmallocHeader *)block;
  if (header.pool != NULL) {
    struct FreeList *free_node = (struct FreeList *)block;
    free_node->next = header.pool->head;
    header.pool->head = free_node;
  } else {
    size_t num_pages =
        header.size / MMU_PAGE_SIZE + !!(header.size % MMU_PAGE_SIZE);
    MMU_free_pages(addr, num_pages);
  }
}
