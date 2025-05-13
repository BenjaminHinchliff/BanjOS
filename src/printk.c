#include "printk.h"
#include "serial.h"
#include "vga.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

int print_char(char c) {
  VGA_display_char(c);
  SER_write(&c, 1);
  return 1;
}

#define PRINT_UNSIGNED(postfix, dtype)                                         \
  int print_unsigned_##postfix(dtype n) {                                      \
    size_t size = 0;                                                           \
    char buf[32]; /* safe - max length of ull is 20 chars */                   \
    do {                                                                       \
      buf[size] = '0' + n % 10;                                                \
      size += 1;                                                               \
      n /= 10;                                                                 \
    } while (n > 0);                                                           \
    for (int i = size - 1; i >= 0; i -= 1) {                                   \
      print_char(buf[i]);                                                      \
    }                                                                          \
    return size;                                                               \
  }                                                                            \
  int print_unsigned_##postfix(dtype n)

#define PRINT_HEX(postfix, dtype)                                              \
  int print_hex_##postfix(dtype n) {                                           \
    size_t size = 0;                                                           \
    char buf[32]; /* safe - max length of ull is 20 chars */                   \
    do {                                                                       \
      if (n % 16 < 10) {                                                       \
        buf[size] = '0' + n % 16;                                              \
      } else {                                                                 \
        buf[size] = 'a' + n % 16 - 10;                                         \
      }                                                                        \
      size += 1;                                                               \
      n /= 16;                                                                 \
    } while (n > 0);                                                           \
    for (int i = size - 1; i >= 0; i -= 1) {                                   \
      print_char(buf[i]);                                                      \
    }                                                                          \
    return size;                                                               \
  }                                                                            \
  int print_hex_##postfix(dtype n)

#define PRINT_SIGNED(postfix, dtype)                                           \
  int print_##postfix(dtype n) {                                               \
    if (n < 0) {                                                               \
      VGA_display_char('-');                                                   \
      return 1 + print_unsigned_##postfix(-n);                                 \
    } else {                                                                   \
      return print_unsigned_##postfix(n);                                      \
    }                                                                          \
  }                                                                            \
  int print_##postfix(dtype n)

#define PRINT_SIZED(postfix, dtype)                                            \
  int print_sized_##postfix(dtype n, char specifier) {                         \
    switch (specifier) {                                                       \
    case 'd':                                                                  \
      return print_##postfix(n);                                               \
    case 'u':                                                                  \
      return print_unsigned_##postfix(n);                                      \
    case 'x':                                                                  \
      return print_hex_##postfix(n);                                           \
    default:                                                                   \
      return 0;                                                                \
    }                                                                          \
  }                                                                            \
  int print_sized_##postfix(dtype n, char specifier)

// uint types
PRINT_UNSIGNED(int, unsigned int);
PRINT_UNSIGNED(short, unsigned short);
PRINT_UNSIGNED(long, unsigned long);
PRINT_UNSIGNED(long_long, unsigned long long);

// int types
PRINT_SIGNED(int, int);
PRINT_SIGNED(short, short);
PRINT_SIGNED(long, long);
PRINT_SIGNED(long_long, long long);

// hex printing
PRINT_HEX(int, unsigned int);
PRINT_HEX(short, unsigned short);
PRINT_HEX(long, unsigned long);
PRINT_HEX(long_long, long long);

// pointers
PRINT_HEX(pointer, uintptr_t);

// sized specifiers (h, l, q)
PRINT_SIZED(short, short);
PRINT_SIZED(long, long);
PRINT_SIZED(long_long, long long);

int print_percent(void) { return print_char('%'); }

int print_string(const char *s) {
  int printed = 0;
  while (*s != '\0') {
    printed += print_char(*s++);
  }
  return printed;
}

/**
 * Supports:
 * - %% '%' literally
 * - %d signed decimal integer
 * - %u unsigned decimal integer
 * - %x unsigned hex integer
 * - %c character
 * - %p pointer address (as hex)
 * - %h[dux] short format
 * - %l[dux] long format
 * - %q[dux] long long? format (prefer ll)
 * - %s
 */
int printk(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t printed = 0;

  while (*fmt != '\0') {
    if (*fmt == '%') {
      fmt += 1;
      switch (*fmt) {
      case '%':
        printed += print_percent();
        break;
      case 's':
        printed += print_string(va_arg(args, const char *));
        break;
      case 'c':
        printed += print_char(va_arg(args, int));
        break;
      case 'd':
        printed += print_int(va_arg(args, int));
        break;
      case 'u':
        printed += print_unsigned_int(va_arg(args, unsigned int));
        break;
      case 'x':
        printed += print_hex_int(va_arg(args, unsigned int));
        break;
      case 'p':
        printed += print_hex_pointer(va_arg(args, uintptr_t));
        break;
      case 'h':
        fmt += 1;
        printed += print_sized_short((short)va_arg(args, int), *fmt);
        break;
      case 'l':
        fmt += 1;
        printed += print_sized_long(va_arg(args, long), *fmt);
        break;
      case 'q':
        fmt += 1;
        printed += print_sized_long_long(va_arg(args, long long), *fmt);
        break;
      default:
        printed += print_char('%');
        printed += print_char(*fmt);
        break;
      }
    } else {
      printed += print_char(*fmt);
    }
    fmt += 1;
  }

  return printed;
}
