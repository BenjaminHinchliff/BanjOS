#include "multiboot_tags.h"
#include "alignment.h"
#include "page_allocator.h"
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

struct TagMemMapEntry {
  uint64_t starting_addr;
  uint64_t length;
  uint32_t type;
  uint32_t : 32; // reserved
} __attribute__((__packed__));

struct TagMemMapEntries {
  struct TagMemMapEntry *d;
  size_t size;
};

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
  uint64_t segment_offset;
  uint64_t segment_size;
  uint32_t table_index_link;
  uint32_t extra_info;
  uint64_t address_alignment;
  uint64_t fixed_entry_size_iff;
} __attribute__((__packed__));

struct TagElfEntries {
  struct TagELFEntry *d;
  size_t size;
};

static struct MemRegions mem_regions;

static bool is_tags_terminator(struct TagCommonHeader *tag) {
  return tag->type == 0 && tag->size == 8;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void add_region(void *start, void *end) {
  assert(mem_regions.size < MEM_REGIONS_LEN &&
         "Number of memory regions must fit into static regions table");
  // align region to pages
  start = (void *)align_pointer((uintptr_t)start, MMU_PAGE_SIZE, true);
  end = (void *)align_pointer((uintptr_t)end, MMU_PAGE_SIZE, false);
  // alignment may mean that the region now is empty if smaller than 2MiB, we
  // don't care about it then
  if (start < end) {
    struct MemRegion *region = &mem_regions.d[mem_regions.size++];
    region->start = (void *)start;
    region->end = (void *)end;
    region->next = (void *)start;
  }
}

static void build_mem_regions(struct TagMemMapEntries mem_entries,
                              struct TagElfEntries elf_entries) {
  size_t i = 0;
  size_t j = 0;
  while (i < mem_entries.size) {
    // skip over regions that aren't type 1
    while (i < mem_entries.size && mem_entries.d[i].type != 1) {
      ++i;
    }
    if (i == mem_entries.size) {
      break;
    }

    uint64_t start = mem_entries.d[i].starting_addr;
    // null is not a valid address
    if (start == (uint64_t)NULL) {
      ++start;
    }

    uint64_t end = (mem_entries.d[i].starting_addr + mem_entries.d[i].length);

    // skip over any disallowed regions before the allowed region
    while (j < elf_entries.size &&
           (elf_entries.d[j].section_type == 0 ||
            elf_entries.d[j].segment_address + elf_entries.d[j].segment_size <=
                start)) {
      ++j;
    }

    uint64_t cur_start = start;
    while (j < elf_entries.size && elf_entries.d[j].segment_address < end) {
      if (elf_entries.d[j].segment_address > cur_start) {
        add_region((void *)cur_start,
                   (void *)MIN(elf_entries.d[j].segment_address, end));
      }

      cur_start = MAX(cur_start, elf_entries.d[j].segment_address +
                                     elf_entries.d[j].segment_size);

      if (cur_start >= end) {
        break;
      }

      ++j;
    }

    if (cur_start < mem_entries.d[i].starting_addr + mem_entries.d[i].length) {
      add_region((void *)cur_start, (void *)end);
    }

    ++i;
  }
}

void multiboot_tags_parse_to_mem_regions() {
  struct TagsHeader *header = multiboot_tag_addr;
  printk("multiboot headaer size: %u\n", header->size);

  struct TagMemMapEntries mem_entries;
  struct TagElfEntries elf_entries;
  struct TagCommonHeader *curtag = (struct TagCommonHeader *)(header + 1);
  while (!is_tags_terminator(curtag)) {
    printk("found tag of type: %u, size %u\n", curtag->type, curtag->size);
    if (curtag->type == TAG_MEM_MAP) {
      struct TagMemMapHeader *memheader = (struct TagMemMapHeader *)curtag;
      printk("mem header entry size: %u, version: %u\n", memheader->entry_size,
             memheader->entry_version);
      assert(memheader->entry_size == sizeof(struct TagMemMapEntry) &&
             "Entry size must map entry struct size");
      mem_entries.d = (struct TagMemMapEntry *)(memheader + 1);
      mem_entries.size = (memheader->size - sizeof(struct TagMemMapHeader)) /
                         memheader->entry_size;
    } else if (curtag->type == TAG_ELF_HEADER) {
      struct TagELFHeader *elfheader = (struct TagELFHeader *)curtag;
      elf_entries.d = (struct TagELFEntry *)(elfheader + 1);
      elf_entries.size = elfheader->num_entries;
    }
    // align to next 8-byte boundary (if needed)
    curtag = (struct TagCommonHeader *)align_pointer(
        (uintptr_t)((uint8_t *)curtag + curtag->size), 8, true);
  }

  printk("Parsed tags mem: %lu elf: %lu\n", mem_entries.size, elf_entries.size);
  /* for (size_t i = 0; i < mem_entries.size; ++i) { */
  /*   struct TagMemMapEntry *e = &mem_entries.d[i]; */
  /*   if (e->type == 1) { */
  /*     printk("mem entry %lu: start: %lx end: %lx\n", i, e->starting_addr, */
  /*            e->starting_addr + e->length); */
  /*   } */
  /* } */
  /* for (size_t i = 0; i < elf_entries.size; ++i) { */
  /*   struct TagELFEntry *e = &elf_entries.d[i]; */
  /*   if (e->section_type != 0) { */
  /*     printk("elf entry %lu: start: %lx end: %lx\n", i, e->segment_address,
   */
  /*            e->segment_address + e->segment_size); */
  /*   } */
  /* } */

  build_mem_regions(mem_entries, elf_entries);
  printk("Num regions: %lu\n", mem_regions.size);
  for (size_t i = 0; i < mem_regions.size; ++i) {
    struct MemRegion *r = &mem_regions.d[i];
    printk("region %lu: start: %lx end: %lx\n", i, (uintptr_t)r->start,
           (uintptr_t)r->end);
  }
}

struct MemRegions *multiboot_get_mem_regions() { return &mem_regions; }
