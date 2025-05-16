#pragma once

#include <stddef.h>

struct MemRegion {
  void *start;
  void *end;
  void *next;
};

#define MEM_REGIONS_LEN 16
struct MemRegions {
  struct MemRegion d[MEM_REGIONS_LEN];
  size_t size;
};

void multiboot_tags_parse_to_mem_regions();
struct MemRegions *multiboot_get_mem_regions();
