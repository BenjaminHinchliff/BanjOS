#include "fs.h"
#include "allocator.h"

struct FSImpl {
  FS_detect_cb probe;
  struct FSImpl *next;
};

static struct FSImpl *impls = NULL;

void FS_register(FS_detect_cb probe) {
  struct FSImpl *impl = kmalloc(sizeof(*impl));
  impl->probe = probe;
  impl->next = impls;
  impls = impl;
}

struct SuperBlock *FS_probe(struct BlockDevice *dev) {
  struct FSImpl *impl = impls;
  while (impl != NULL) {
    struct SuperBlock *sb = impl->probe(dev);
    if (sb != NULL) {
      return sb;
    }
    impl = impl->next;
  }
  return NULL;
}
