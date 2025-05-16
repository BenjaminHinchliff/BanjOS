#pragma once

// 4 KiB pages
#define MMU_PAGE_SIZE 0x1000

void *MMU_pf_alloc();
void MMU_pf_free(void *pf);
#ifdef MMU_MEMTEST
void MMU_memtest();
#endif
