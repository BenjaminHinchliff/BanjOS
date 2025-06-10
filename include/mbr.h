#pragma once

#include "block_device.h"

#include <stdint.h>

struct MBRPartition {
  uint8_t status;
  uint8_t first_sector_head;
  uint8_t first_sector_high;
  uint8_t first_sector_low;
  uint8_t part_type;
  uint8_t last_sector_head;
  uint8_t last_sector_high;
  uint8_t last_sector_low;
  uint32_t first_sector_lba;
  uint32_t num_sectors;
} __attribute__((packed));

_Static_assert(sizeof(struct MBRPartition) == 16,
               "MBR Partition Entries should be 16 bytes in size");

struct MBR {
  uint8_t bootstrap_code[446];
  struct MBRPartition partitions[4];
  uint16_t boot_signature;
} __attribute__((packed));

_Static_assert(sizeof(struct MBR) == 512, "MBR should be one block in size");

int mbr_init(struct MBR *mbr, struct BlockDevice *dev);
int MBR_read_block(struct BlockDevice *dev, struct MBR *mbr, uint8_t part_num,
                   uint64_t blk_num, void *dst);
