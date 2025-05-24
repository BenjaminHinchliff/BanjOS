#pragma once

#include <stddef.h>

// 4 KiB pages
#define MMU_PAGE_SIZE 0x1000

size_t get_free_memory();

// TODO: should these be exported nowadays
void *MMU_pf_alloc();
void MMU_pf_free(void *pf);

void MMU_alloc_init();
void *MMU_alloc_page();
void *MMU_alloc_pages(int num);
void MMU_free_page(void *addr);
void MMU_free_pages(void *addr, int num);

#ifdef MMU_MEMTEST
void MMU_memtest();
#endif
