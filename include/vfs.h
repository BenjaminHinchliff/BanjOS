#pragma once

#include <stdint.h>

struct Inode;
typedef int (*readdir_cb)(const char *, struct Inode *, void *);

struct File {
  int (*close)(struct File **file);
  int (*read)(struct File *file, char *dst, int len);
  int (*write)(struct File *file, char *dst, int len);
  int (*lseek)(struct File *file, off_t offset);
  int (*mmap)(struct File *file, void *addr);
};

struct Inode {
  ino_t ino;
  mode_t st_mode;
  uid_t st_uid;
  gid_t st_gid;
  off_t st_size;
  struct File *(*open)(struct Inode *inode);
  int (*readdir)(struct Inode *inode, readdir_cb cb, void *p);
  int (*unlink)(struct Inode *inode, const char *name);
};

struct SuperBlock {
  const char *name;
  const char *type;
  struct Inode *root_inode;
  struct Inode *(*read_inode)(struct SuperBlock *sb, unsigned long inode_num);
  int (*sync_fs)(struct SuperBlock *);
  void (*put_super)(struct SuperBlock *);
};
