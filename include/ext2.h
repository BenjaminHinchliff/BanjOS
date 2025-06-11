#pragma once

#include <stdint.h>

typedef uint32_t ino_t;
typedef uint16_t mode_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint64_t off_t;

#include "vfs.h"

void ext2_init();
