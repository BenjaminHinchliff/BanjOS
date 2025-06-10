#include "mbr.h"
#include "block_device.h"
#include "smolassert.h"

int mbr_init(struct MBR *mbr, struct BlockDevice *dev) {
  return dev->read_block(dev, 0, mbr);
}

int MBR_read_block(struct BlockDevice *dev, struct MBR *mbr, uint8_t part_num,
                   uint64_t blk_num, void *dst) {
  assert(part_num < 4 && "Driver only supports up to 4 primary partitions!");
  return dev->read_block(
      dev, mbr->partitions[part_num].first_sector_lba + blk_num, dst);
}
