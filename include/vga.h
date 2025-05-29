#pragma once

#include <stdint.h>

#define VGA_START (volatile uint16_t *)0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)
#define VGA_SCREEN_CHAR_BYTES 2

#define VGA_BLACK 0x00
#define VGA_BLUE 0x01
#define VGA_GREEN 0x02
#define VGA_CYAN 0x03
#define VGA_RED 0x04
#define VGA_PURPLE 0x05
#define VGA_ORANGE 0x06
#define VGA_LIGHT_GREY 0x07
#define VGA_DARK_GREY 0x08
#define VGA_BRIGHT_BLUE 0x09
#define VGA_BRIGHT_GREEN 0x0A
#define VGA_BRIGHT_CYAN 0x0B
#define VGA_MAGENTA 0x0C
#define VGA_BRIGHT_PURPLE 0x0D
#define VGA_YELLOW 0x0E
#define VGA_WHITE 0x0F

int VGA_row_count(void);
int VGA_col_count(void);
void VGA_clear(void);
void VGA_display_char(char c);
void VGA_display_attr_char(int x, int y, char c, int fg, int bg);
void VGA_display_str(const char *s);
