#pragma once

#include "processes.h"
#include <stdbool.h>
#include <stdint.h>

struct BlockDevice {
  int (*read_block)(struct BlockDevice *this, uint64_t blk_num, void *dst);
  uint32_t blk_size;
  uint64_t tot_length;
};

struct ATARequest {
  uint64_t blk_num;
  struct ProcessQueue block_queue;
  struct ATARequest *next;
};

struct ATABlockDevice {
  struct BlockDevice dev;
  uint16_t ata_base, ata_master;
  uint8_t slave, irq;
  struct ATARequest *req_head, *req_tail;
};

#define PRIM_IO_BASE 0x1F0
#define PRIM_CTL_BASE 0x3F6

enum ATADevType {
  ATADEV_PATAPI,
  ATADEV_SATAPI,
  ATADEV_PATA,
  ATADEV_SATA,
  ATADEV_UNKNOWN,
};

enum ATADevType detect_devtype(uint8_t slavebit, uint16_t base,
                               uint16_t ctl_base);
struct BlockDevice *ata_probe(uint16_t base, uint16_t master, uint8_t slave,
                              uint8_t irq);

int BLK_register(struct BlockDevice *dev);
