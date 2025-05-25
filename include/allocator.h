#pragma once

#include <stddef.h>

void init_alloc();
void *kmalloc(size_t size);
void kfree(void *addr);
