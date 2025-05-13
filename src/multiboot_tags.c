#include "printk.h"
#include "smolassert.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// set during boot in boot.asm
extern void *multiboot_tag_addr;

struct TagsHeader {
  uint32_t size;
  uint32_t : 32; // reserved
} __attribute__((__packed__));

#define TAG_MEM_MAP 6
#define TAG_ELF_HEADER 9

struct TagCommonHeader {
  uint32_t type;
  uint32_t size;
} __attribute__((__packed__));

struct TagMemMapHeader {
  uint32_t type; // should be 6
  uint32_t size;
  uint32_t entry_size;
  uint32_t entry_version; // should be 0
} __attribute__((__packed__));

struct TagELFHeader {
  uint32_t type; // should be 9
  uint32_t size;
  uint32_t num_entries;
  uint32_t entry_size;
  uint32_t string_table_index;
} __attribute__((__packed__));

struct TagELFEntry {
  uint32_t section_name;
  uint32_t section_type;
  uint64_t flags;
  uint64_t segment_address;
  uint64_t segment_size;
  uint32_t table_index_link;
  uint32_t extra_info;
  uint64_t address_alignment;
  uint64_t fixed_entry_size_iff;
} __attribute__((__packed__));

struct TagMemMapEntry {
  uint64_t starting_addr;
  uint64_t length;
  uint32_t type;
  uint32_t : 32; // reserved
} __attribute__((__packed__));

static bool is_tags_terminator(struct TagCommonHeader *tag) {
  return tag->type == 0 && tag->size == 8;
}

static uintptr_t align_pointer(uintptr_t addr, uint32_t boundary) {
  if (addr % 8 != 0) {
    return addr + (8 - addr % 8);
  } else {
    return addr;
  }
}

static void memmap_parse(struct TagMemMapEntry *entry, int num_entries) {
  while (num_entries--) {
    if (entry->type == 1) {
      printk("%d: mem tag: type: %d length: %lx addr %lx\n", num_entries,
             entry->type, entry->length, entry->starting_addr);
    }
    entry++;
  }
}

static void elf_parse(struct TagELFEntry *entry, int num_entries) {
  while (num_entries--) {
    entry;
    entry++;
  }
}

void multiboot_tags_parse() {
  struct TagsHeader *header = multiboot_tag_addr;
  printk("multiboot headaer size: %u\n", header->size);
  struct TagCommonHeader *curtag = (struct TagCommonHeader *)(header + 1);
  while (!is_tags_terminator(curtag)) {
    printk("found tag of type: %u, size %u\n", curtag->type, curtag->size);
    if (curtag->type == TAG_MEM_MAP) {
      struct TagMemMapHeader *memheader = (struct TagMemMapHeader *)curtag;
      printk("mem header entry size: %u, version: %u\n", memheader->entry_size,
             memheader->entry_version);
      assert(memheader->entry_size == sizeof(struct TagMemMapEntry) &&
             "Entry size must map entry struct size");
      memmap_parse((struct TagMemMapEntry *)(memheader + 1),
                   (memheader->size - sizeof(struct TagMemMapHeader)) /
                       memheader->entry_size);
    } else if (curtag->type == TAG_ELF_HEADER) {
      struct TagELFHeader *elfheader = (struct TagELFHeader *)curtag;
      elf_parse((struct TagELFEntry *)(elfheader + 1), elfheader->num_entries);
    }
    // align to next 8-byte boundary (if needed)
    curtag = (struct TagCommonHeader *)align_pointer(
        (uintptr_t)((uint8_t *)curtag + curtag->size), 8);
  }
}
