#include "ext2.h"
#include "alignment.h"
#include "allocator.h"
#include "block_device.h"
#include "fs.h"
#include "mbr.h"
#include "printk.h"
#include "vfs.h"

#include <stdint.h>
#include <string.h>

struct Ext2SuperBlock {
  uint32_t num_inodes;
  uint32_t num_blocks;
  uint32_t num_reserved_blocks;
  uint32_t num_unallocated_blocks;
  uint32_t num_unallocated_inodes;
  uint32_t sb_block_num;
  uint32_t log_sub_10_block_size;
  uint32_t log_sub_10_fragment_size;
  uint32_t num_group_blocks;
  uint32_t num_group_fragments;
  uint32_t num_group_inodes;
  uint32_t last_mount_time;
  uint32_t last_written_time;
  uint16_t mounts_since_last_fsck;
  uint16_t mounts_allowed_before_fsck;
  uint16_t ext2_signature;
  uint16_t file_system_state;
  uint16_t err_action;
  uint16_t minor_version_num;
  uint32_t last_fsck_time;
  uint32_t max_time_between_fsck;
  uint32_t operating_system_id;
  uint32_t major_version_number;
  uint16_t uid_reserved;
  uint16_t gid_reserved;
  uint32_t first_non_reserved_inode;
  uint32_t inode_size;
} __attribute__((packed));

_Static_assert(sizeof(struct Ext2SuperBlock) == 92,
               "Superblock should be size 92");

struct Ext2BlockGroupDescriptorTable {
  uint32_t block_addr_block_usage_bitmap;
  uint32_t block_addr_inode_usage_bitmap;
  uint32_t start_block_addr_inode_table;
  uint16_t num_unallocated_blocks;
  uint16_t num_unallocated_inodes;
  uint16_t num_directories;
  uint8_t padding[14];
} __attribute__((packed));

_Static_assert(sizeof(struct Ext2BlockGroupDescriptorTable) == 32,
               "Ext2 block descriptor header size 32");

struct Ext2Inode {
  uint16_t mode;
  uint16_t uid;
  uint32_t size_low;
  uint32_t last_access;
  uint32_t ctime;
  uint32_t mtime;
  uint32_t dtime;
  uint16_t gid;
  uint16_t hard_link_count;
  uint32_t sector_count;
  uint32_t flags;
  uint32_t os_specific;
  uint32_t direct_blocks[12];
  uint32_t singly_indirect_block;
  uint32_t doubly_indirect_block;
  uint32_t triply_indirect_block;
  uint32_t nfs_generation_num;
  uint32_t acl;
  uint32_t size_high;
} __attribute__((packed));

#define NUM_DIRECT_BLOCKS                                                      \
  (sizeof(((struct Ext2Inode *)0)->direct_blocks) / sizeof(uint32_t))

struct Ext2DirEntryHeader {
  uint32_t ino;
  uint16_t entry_size;
  uint8_t name_len_lsb;
  uint8_t msb_name_len_or_type;
  const char name;
} __attribute__((packed));

_Static_assert(sizeof(struct Ext2Inode) == 112, "Inodes should be 112 bytes");

#define EXT2_OFFSET 1024
#define EXT2_SB_SIZE 1024

void ext2_superblock_init(struct Ext2SuperBlock *sb,
                          struct Ext2BlockGroupDescriptorTable *grp_table,
                          struct MBR *mbr, struct BlockDevice *dev,
                          uint8_t num_part) {
  uint64_t sb_start = EXT2_OFFSET / dev->blk_size;
  uint16_t sb_sectors = EXT2_SB_SIZE / dev->blk_size;
  for (int i = 0; i < sb_sectors; ++i) {
    MBR_read_block(dev, mbr, num_part, sb_start + i,
                   (void *)sb + dev->blk_size * i);
  }
  // read group table after superblock too
  MBR_read_block(dev, mbr, num_part, sb_start + sb_sectors, grp_table);
}

struct Ext2VfsSuperBlock {
  struct SuperBlock sb;
  uint64_t block_size;
  uint64_t num_groups;
  struct BlockDevice *dev;
  struct MBR *mbr;
  uint8_t part_num;
  struct Ext2BlockGroupDescriptorTable *grp_table;
  struct Ext2SuperBlock *ext_sb;
};

uint64_t pow2(uint64_t n) {
  uint64_t number = 1;
  for (uint64_t i = 0; i < n; ++i) {
    number *= 2;
  }
  return number;
}

int ext2_read_block(struct Ext2VfsSuperBlock *vsb, off_t block_num, void *dst) {
  off_t sector_num = block_num * (vsb->block_size / vsb->dev->blk_size);
  off_t num_sectors = vsb->block_size / vsb->dev->blk_size;

  for (off_t i = 0; i < num_sectors; ++i) {
    if (!MBR_read_block(vsb->dev, vsb->mbr, vsb->part_num, sector_num + i,
                        dst + vsb->dev->blk_size * i)) {
      return false;
    }
  }
  return true;
}

struct Ext2VfsInode {
  struct Inode in;
  struct Ext2Inode *ext_in;
  struct Ext2VfsSuperBlock *vsb;
};

void read_inode_block(struct Ext2VfsInode *vino, uint64_t block, void *dst) {
  uint64_t num_indirect_per = vino->vsb->block_size / sizeof(uint32_t);
  if (block < NUM_DIRECT_BLOCKS) {
    ext2_read_block(vino->vsb, vino->ext_in->direct_blocks[block], dst);
  } else if (block - NUM_DIRECT_BLOCKS < num_indirect_per) {
    uint32_t *indirect_block = kmalloc(vino->vsb->block_size);
    ext2_read_block(vino->vsb, vino->ext_in->singly_indirect_block,
                    indirect_block);
    ext2_read_block(vino->vsb, indirect_block[block - NUM_DIRECT_BLOCKS], dst);
  } else if (block - NUM_DIRECT_BLOCKS - num_indirect_per <
             num_indirect_per * num_indirect_per) {
    uint64_t off = block - NUM_DIRECT_BLOCKS - num_indirect_per;
    uint64_t single_off = off / num_indirect_per;
    uint64_t double_off = off % num_indirect_per;
    uint32_t *indirect_block = kmalloc(vino->vsb->block_size);
    ext2_read_block(vino->vsb, vino->ext_in->singly_indirect_block,
                    indirect_block);
    uint32_t *indirecter_block = kmalloc(vino->vsb->block_size);
    ext2_read_block(vino->vsb, indirect_block[single_off], indirecter_block);
    ext2_read_block(vino->vsb, indirect_block[double_off], dst);
  } else if (block - NUM_DIRECT_BLOCKS - num_indirect_per -
                 num_indirect_per * num_indirect_per <
             num_indirect_per * num_indirect_per * num_indirect_per) {
    uint64_t off = block - NUM_DIRECT_BLOCKS - num_indirect_per -
                   num_indirect_per * num_indirect_per;
    uint64_t single_off = off / (num_indirect_per * num_indirect_per);
    uint64_t double_off = off % (num_indirect_per * num_indirect_per);
    uint64_t triple_off = double_off / num_indirect_per;
    uint64_t quad_off = double_off % num_indirect_per;
    uint32_t *indirect_block = kmalloc(vino->vsb->block_size);
    ext2_read_block(vino->vsb, vino->ext_in->doubly_indirect_block,
                    indirect_block);
    uint32_t *indirecter_block = kmalloc(vino->vsb->block_size);
    ext2_read_block(vino->vsb, indirect_block[single_off], indirecter_block);
    uint32_t *indirecterer_block = kmalloc(vino->vsb->block_size);
    ext2_read_block(vino->vsb, indirecter_block[triple_off], indirecter_block);
    ext2_read_block(vino->vsb, indirect_block[quad_off], dst);
  }
}

int readdir(struct Inode *inode, readdir_cb cb, void *arg) {
  struct Ext2VfsInode *vino = (struct Ext2VfsInode *)inode;
  if (!(inode->st_mode & 0x4000)) {
    return false;
  }

  uint64_t num_blocks = vino->in.st_size / vino->vsb->block_size +
                        !!(vino->in.st_size % vino->vsb->block_size);
  void *content_block = kmalloc(vino->vsb->block_size);
  for (int i = 0; i < num_blocks; ++i) {
    read_inode_block(vino, i, content_block);
    struct Ext2DirEntryHeader *header = content_block;
    while ((void *)header < content_block + vino->vsb->block_size &&
           header->ino != 0) {
      char *name = kmalloc(sizeof(*name) * header->name_len_lsb + 1);
      int i;
      for (i = 0; i < header->name_len_lsb; ++i) {
        name[i] = ((const char *)&header->name)[i];
      }
      name[i] = '\0';
      struct Inode *entry_ino =
          vino->vsb->sb.read_inode((struct SuperBlock *)vino->vsb, header->ino);
      cb(name, entry_ino, arg);
      kfree(name);
      header = (struct Ext2DirEntryHeader *)align_pointer(
          (uintptr_t)((void *)header + header->entry_size), 4, true);
    }
  }

  return true;
}

struct Ext2VfsInode *ext2_vfs_inode_init(struct Ext2Inode *ext_in, ino_t ino,
                                         struct Ext2VfsSuperBlock *vsb) {
  struct Ext2VfsInode *vin = kmalloc(sizeof(*vin));
  vin->in.ino = ino;
  vin->in.st_mode = ext_in->mode;
  vin->in.st_uid = ext_in->uid;
  vin->in.st_gid = ext_in->gid;
  vin->in.st_size = ((off_t)ext_in->size_high << 32) | ext_in->size_low;
  vin->in.open = NULL;
  vin->in.readdir = &readdir;
  vin->in.unlink = NULL;
  vin->ext_in = ext_in;
  vin->vsb = vsb;
  return vin;
}

struct Inode *read_inode(struct SuperBlock *sb, unsigned long inode_num) {
  struct Ext2VfsSuperBlock *vsb = (struct Ext2VfsSuperBlock *)sb;
  if (inode_num > vsb->ext_sb->num_inodes) {
    return NULL;
  }

  off_t group_idx = (inode_num - 1) / vsb->ext_sb->num_group_inodes;
  if (group_idx >= vsb->num_groups) {
    return NULL;
  }

  struct Ext2BlockGroupDescriptorTable *group = &vsb->grp_table[group_idx];

  off_t index_idx = (inode_num - 1) % vsb->ext_sb->num_group_inodes;
  off_t block_idx = index_idx * vsb->ext_sb->inode_size / vsb->block_size;
  off_t block_offset = (index_idx * vsb->ext_sb->inode_size) % vsb->block_size;
  off_t block_num = group->start_block_addr_inode_table + block_idx;

  off_t sector_num = block_num * (vsb->block_size / vsb->dev->blk_size);
  off_t num_sectors = vsb->block_size / vsb->dev->blk_size;

  void *ext_inode_block = kmalloc(vsb->block_size);
  for (off_t i = 0; i < num_sectors; ++i) {
    MBR_read_block(vsb->dev, vsb->mbr, vsb->part_num, sector_num + i,
                   ext_inode_block + vsb->dev->blk_size * i);
  }
  struct Ext2Inode *ext_inode = kmalloc(sizeof(*ext_inode));
  memcpy(ext_inode, ext_inode_block + block_offset, sizeof(*ext_inode));
  kfree(ext_inode_block);
  return (struct Inode *)ext2_vfs_inode_init(ext_inode, inode_num, vsb);
}

struct Ext2VfsSuperBlock *
ext2_vfs_superblock_init(struct Ext2SuperBlock *ext_sb,
                         struct Ext2BlockGroupDescriptorTable *grp_table,
                         struct BlockDevice *dev, struct MBR *mbr,
                         uint8_t part_num) {
  struct Ext2VfsSuperBlock *vsb = kmalloc(sizeof(*vsb));
  vsb->sb.name = "Ext2";
  vsb->sb.type = "ext2";
  vsb->sb.read_inode = &read_inode;
  vsb->sb.sync_fs = NULL;
  vsb->sb.put_super = NULL;
  vsb->ext_sb = ext_sb;
  vsb->block_size = pow2(vsb->ext_sb->log_sub_10_block_size + 10);
  vsb->num_groups = vsb->ext_sb->num_blocks / vsb->ext_sb->num_group_blocks +
                    !!(vsb->ext_sb->num_blocks % vsb->ext_sb->num_group_blocks);
  vsb->grp_table = grp_table;
  vsb->dev = dev;
  vsb->mbr = mbr;
  vsb->part_num = part_num;
  vsb->sb.root_inode = read_inode((struct SuperBlock *)vsb, 2);
  return vsb;
}

struct SuperBlock *ext2_probe(struct BlockDevice *dev) {
  struct MBR *mbr = kmalloc(sizeof(*mbr));
  mbr_init(mbr, dev);
  for (int p = 0; p < MAX_MBR_PARTITIONS && mbr->partitions[p].part_type != 0;
       ++p) {
    if (mbr->partitions[p].num_sectors >= 2) {
      struct Ext2SuperBlock *sb = kmalloc(EXT2_SB_SIZE);
      struct Ext2BlockGroupDescriptorTable *grp_table = kmalloc(dev->blk_size);
      ext2_superblock_init(sb, grp_table, mbr, dev, p);
      if (sb->ext2_signature == 0xEF53) {
        return (struct SuperBlock *)ext2_vfs_superblock_init(sb, grp_table, dev,
                                                             mbr, p);
      }
    }
  }
  return NULL;
}

void ext2_init() { FS_register(ext2_probe); }
