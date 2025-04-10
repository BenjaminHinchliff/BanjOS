#include "vga.h"

#include <stddef.h>
#include <string.h>

#define LINE(cursor) (cursor / VGA_WIDTH)
#define COLUMN(cursor) (cursor % VGA_WIDTH)

static volatile uint16_t *VGA_buf = VGA_START;
static size_t VGA_cursor = 0;

void VGA_clear(void) {
  memset((uint16_t *)VGA_buf, 0x0f << 8, VGA_WIDTH * VGA_HEIGHT);
}

void VGA_display_char(char c) {
  if (c == '\n') {
    VGA_cursor = (LINE(VGA_cursor) + 1) * VGA_WIDTH;
    if (VGA_cursor >= VGA_WIDTH * VGA_HEIGHT) {
      VGA_scroll();
    }
  } else if (c == '\r') {
    VGA_cursor = LINE(VGA_cursor);
  } else {
    VGA_buf[VGA_cursor] = (0x0f << 8) | c;
    if (COLUMN(VGA_cursor) < (VGA_WIDTH - 1)) {
      VGA_cursor += 1;
    } else {
      VGA_display_char('\n');
    }
  }
}

void VGA_display_str(const char *s) {
  while (*s != '\0') {
    VGA_display_char(*s);
    s += 1;
  }
}

void VGA_scroll(void) {
  const size_t one_line = VGA_WIDTH;
  // ideally I'd have memmove or something but that's difficult rn
  for (size_t i = 0; i < VGA_SIZE - one_line; i += 1) {
    VGA_buf[i] = VGA_buf[i + one_line];
  }
  memset((uint16_t *)(VGA_buf + VGA_SIZE - one_line), 0, VGA_WIDTH * VGA_SCREEN_CHAR_BYTES);
  VGA_cursor = (LINE(VGA_cursor) - 1) * VGA_WIDTH;
}
