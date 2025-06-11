#include "block_device.h"
#include "allocator.h"
#include "interrupts.h"
#include "portio.h"
#include "printk.h"
#include "processes.h"
#include "smolassert.h"
#include "string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define REG_DATA 0
#define REG_ERR 1
#define REG_SEC_CNT 2
#define REG_SEC_NUM 3
#define REG_CYL_LO 4
#define REG_CYL_HI 5
#define REG_DEVSEL 6
#define REG_STATUS 7
#define REG_CMD 7

#define REG_ALT_STS 0
#define REG_DEV_CTL 0
#define REG_DRV_ADDR 1

#define CMD_IDENTIFY 0xEC
#define CMD_READ_SECTORS_EXT 0x24

#define STATUS_ERR (1 << 0)
#define STATUS_IDX (1 << 1)
#define STATUS_COR (1 << 2)
#define STATUS_DRQ (1 << 3)
#define STATUS_SRV (1 << 4)
#define STATUS_DF (1 << 5)
#define STATUS_RDY (1 << 6)
#define STATUS_BSY (1 << 7)

#define BLOCK_SIZE 512

void ata_soft_reset(uint16_t ctl_base, bool interrupts) {
  outb(ctl_base + REG_DEV_CTL, 0x4);
  outb(ctl_base + REG_DEV_CTL, 0x0 | !interrupts);
}

enum ATADevType detect_devtype(uint8_t slavebit, uint16_t base,
                               uint16_t ctl_base) {
  ata_soft_reset(ctl_base, false); /* waits until master drive is ready again */
  outb(base + REG_DEVSEL, 0xA0 | slavebit << 4);
  inb(ctl_base); /* wait 400ns for drive select to work */
  inb(ctl_base);
  inb(ctl_base);
  inb(ctl_base);
  unsigned cl = inb(base + REG_CYL_LO); /* get the "signature bytes" */
  unsigned ch = inb(base + REG_CYL_HI);

  /* differentiate ATA, ATAPI, SATA and SATAPI */
  if (cl == 0x14 && ch == 0xEB) {
    return ATADEV_PATAPI;
  } else if (cl == 0x69 && ch == 0x96) {
    return ATADEV_SATAPI;
  } else if (cl == 0 && ch == 0) {
    return ATADEV_PATA;
  } else if (cl == 0x3c && ch == 0xc3) {
    return ATADEV_SATA;
  } else {
    return ATADEV_UNKNOWN;
  }
}

uint64_t dev_identify(uint16_t base) {
  outw(base + REG_SEC_CNT, 0);
  outw(base + REG_SEC_NUM, 0);
  outw(base + REG_CYL_LO, 0);
  outw(base + REG_CYL_HI, 0);
  outw(base + REG_CMD, CMD_IDENTIFY);
  uint8_t status = inb(base + REG_STATUS);
  assert(status != 0 && "ATA Device wasn't present actually");
  while (status & STATUS_BSY) {
    status = inb(base + REG_STATUS);
  }
  /* assert(inw(base + REG_CYL_LO) == 0 && inw(base + REG_CYL_HI) == 0 && */
  /*        "This isn't a real ATA device"); */
  while (!(status & (STATUS_DRQ | STATUS_ERR))) {
    status = inb(base + REG_STATUS);
  }
  assert(!(status & STATUS_ERR) && "Error when reading IDENTIFY data");
  uint16_t *block = kmalloc(BLOCK_SIZE);
  for (size_t i = 0; i < BLOCK_SIZE / sizeof(uint16_t); ++i) {
    block[i] = inw(base + REG_DATA);
  }
  uint64_t sectors = 0;
  for (size_t i = 104; i >= 100; --i) {
    sectors = (sectors << 8) | block[i];
  }
  return sectors;
}

void ata_req_queue(struct ATABlockDevice *ata, struct ATARequest *req) {
  req->next = NULL;
  if (ata->req_tail != NULL) {
    ata->req_tail->next = req;
    ata->req_tail = req;
  } else {
    ata->req_head = req;
    ata->req_tail = req;
  }
}

struct ATARequest *ata_req_unqueue(struct ATABlockDevice *ata) {
  if (ata->req_head == NULL) {
    return NULL;
  }

  struct ATARequest *req = ata->req_head;
  ata->req_head = req->next;
  if (ata->req_head == NULL) {
    ata->req_tail = ata->req_head;
  }
  req->next = NULL;
  return req;
}

void ata_req_execute(struct ATABlockDevice *ata, struct ATARequest *req) {
  /* printk("reading: %lu\n", req->blk_num); */
  /* printk("next req: %lx\n", ata->req_head); */
  uint16_t sector_count = 1;
  outb(ata->ata_base + REG_DEVSEL, 0x40 | ata->slave << 4);
  outb(ata->ata_base + REG_SEC_CNT, sector_count >> 8);
  outb(ata->ata_base + REG_SEC_NUM, (req->blk_num >> 24) & 0xFF);
  outb(ata->ata_base + REG_CYL_LO, (req->blk_num >> 32) & 0xFF);
  outb(ata->ata_base + REG_CYL_HI, (req->blk_num >> 40) & 0xFF);
  outb(ata->ata_base + REG_SEC_CNT, sector_count & 0xFF);
  outb(ata->ata_base + REG_SEC_NUM, req->blk_num & 0xFF);
  outb(ata->ata_base + REG_CYL_LO, (req->blk_num >> 8) & 0xFF);
  outb(ata->ata_base + REG_CYL_HI, (req->blk_num >> 16) & 0xFF);
  outb(ata->ata_base + REG_CMD, CMD_READ_SECTORS_EXT);
}

void ata_req_queue_execute(struct ATABlockDevice *ata, struct ATARequest *req) {
  if (ata->req_head == NULL) {
    ata_req_queue(ata, req);
    ata_req_execute(ata, req);
  } else {
    ata_req_queue(ata, req);
  }
}

void read_block_handler(int number, int error_code, void *arg) {
  struct ATABlockDevice *ata = (struct ATABlockDevice *)arg;
  if (ata->block_queue.head != NULL) {
    PROC_unblock_head(&ata->block_queue);
  }
  PIC_sendEOI(number);
}

int ata_48_read_block(struct BlockDevice *this, uint64_t blk_num, void *dst) {
  struct ATABlockDevice *ata = (struct ATABlockDevice *)this;
  struct ATARequest *req = kmalloc(sizeof(*req));
  req->blk_num = blk_num;
  ata_req_queue_execute(ata, req);
  uint8_t status = inb(ata->ata_master + REG_ALT_STS);
  CLI;
  while ((status & STATUS_BSY) && !(status & (STATUS_DRQ | STATUS_ERR))) {
    PROC_block_on(&ata->block_queue, true);
    CLI;
    status = inb(ata->ata_master + REG_ALT_STS);
  }
  STI;
  for (size_t i = 0; i < ata->dev.blk_size / sizeof(uint16_t); ++i) {
    ((uint16_t *)dst)[i] = inw(ata->ata_base + REG_DATA);
  }

  kfree(ata_req_unqueue(ata));
  if (ata->req_head != NULL) {
    ata_req_execute(ata, ata->req_head);
  }

  return 1;
}

struct BlockDevice *ata_probe(uint16_t base, uint16_t master, uint8_t slave,
                              uint8_t irq) {
  enum ATADevType dev_type = detect_devtype(slave, base, master);
  if (dev_type != ATADEV_PATA) {
    printk("ATA Driver only supports PATA Devices!\n");
    return NULL;
  }
  uint64_t sectors = dev_identify(base);
  assert(sectors > 0 && "ATA device might not support 48 bit addressing");
  printk("Found ATA device with %lx sectors\n", sectors);
  struct ATABlockDevice *ata = kmalloc(sizeof(*ata));
  memset(ata, 0, sizeof(*ata));
  ata->ata_base = base;
  ata->slave = slave;
  ata->ata_master = master;
  ata->irq = irq;
  PROC_init_queue(&ata->block_queue);
  ata->dev.read_block = &ata_48_read_block;
  ata->dev.blk_size = BLOCK_SIZE;
  ata->dev.tot_length = sectors;
  ata_soft_reset(ata->ata_base, true);
  IRQ_handler_set(IRQ_BASE + IRQ14, &read_block_handler, ata);
  IRQ_clear_mask(IRQ2);
  IRQ_clear_mask(IRQ14);
  return (struct BlockDevice *)ata;
}

struct BlockDeviceRegistration {
  struct BlockDevice *dev;
  struct BlockDeviceRegistration *next;
};

static struct BlockDeviceRegistration *registered_devs;

int BLK_register(struct BlockDevice *dev) {
  struct BlockDeviceRegistration *dev_reg = kmalloc(sizeof(*dev_reg));
  dev_reg->dev = dev;
  dev_reg->next = registered_devs;
  registered_devs = dev_reg;
  return 1;
}
