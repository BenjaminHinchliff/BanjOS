#pragma once

#include <stdint.h>
#include <stddef.h>

void GDT_setup();
void GDT_push_dq(uint64_t data);
size_t GDT_push(uint64_t *data, size_t len);
void GDT_load();
