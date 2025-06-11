#pragma once

#include "block_device.h"

typedef struct SuperBlock *(*FS_detect_cb)(struct BlockDevice *dev);

void FS_register(FS_detect_cb probe);
struct SuperBlock *FS_probe(struct BlockDevice *dev);
