#pragma once

#include <stdint.h>

#define VGA_START (volatile uint16_t*)0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)
#define VGA_SCREEN_CHAR_BYTES 2

void VGA_clear(void);
void VGA_display_char(char c);
void VGA_display_str(const char *s);
