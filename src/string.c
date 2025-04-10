#include <stddef.h>
#include <stdint.h>

// could use stos maybe
void *memset(void *dst, int c, size_t n) {
  unsigned char *bdst = dst;

  while (n > 0) {
    *bdst = c;
    bdst += 1;
    n -= 1;
  }

  return dst;
}

void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *bdest = dest;
  const unsigned char *bsrc = src;

  while (n > 0) {
    *bdest = *bsrc;
    bdest += 1;
    bsrc += 1;
    n -= 1;
  }

  return dest;
}

size_t strlen(const char *s) {
  size_t n = 0;
  while (*s != '\0') {
    n += 1;
    s += 1;
  }
  return n;
}

char *strcpy(char *dest, const char *src) {
  const size_t n = strlen(src);
  memcpy(dest, src, n);
  dest[n] = '\0';
  return dest;
}

// I don't wanna write an allocator T^T
// char *strdup(const char *s);
