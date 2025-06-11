#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline uintptr_t align_pointer(uintptr_t addr, uint32_t boundary,
                                      bool after) {
  if (addr % boundary != 0) {
    if (after) {
      return addr + (boundary - addr % boundary);
    } else {
      return addr - addr % boundary;
    }
  } else {
    return addr;
  }
}
