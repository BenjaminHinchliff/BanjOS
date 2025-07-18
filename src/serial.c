#include "serial.h"
#include "exit.h"
#include "interrupts.h"
#include "portio.h"
#include "printk.h"
#include "ring_buffer.h"

#include <stdbool.h>
#include <string.h>

#define COM1 0x3f8
#define INT_ENABLE_TRANSMIT_EMPTY (1 << 1)

static struct RingBuffer ring;

static int is_transmit_empty() { return inb(COM1 + 5) & 0x20; }

static void write_serial(char c) { outb(COM1, c); }

static void serial_ring_consumer_serial_write(struct RingBuffer *state) {
  if (is_transmit_empty()) {
    char c;
    if (ring_consumer_next(state, &c)) {
      write_serial(c);
    }
  }
}

static void serial_transmit_ready_handler(int num, int error_code, void *arg) {
  serial_ring_consumer_serial_write((struct RingBuffer *)arg);
  PIC_sendEOI(num);
}

int SER_write(const char *buff, int len) {
  CLI_GUARD;

  for (int i = 0; i < len; i += 1) {
    ring_producer_add_char(&ring, buff[i]);
  }
  serial_ring_consumer_serial_write(&ring);

  if (are_interrupts_enabled()) {
    STI;
  }

  STI_GUARD;
  return len;
}

void SER_init(void) {
  ring_init(&ring, false);

  outb(COM1 + 1, 0x00); // Disable all interrupts
  outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
  outb(COM1 + 4, 0x1E); // Set in loopback mode, test the serial chip
  outb(COM1 + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial
                        // returns same byte)

  // Check if serial is faulty (i.e: not same byte as sent)
  if (inb(COM1 + 0) != 0xAE) {
    printk("serial chip fault!\n");
    EXIT;
  }

  // If serial is not faulty set it in normal operation mode
  // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
  // setup interrupts
  IRQ_handler_set(IRQ_BASE + IRQ4, serial_transmit_ready_handler, &ring);
  IRQ_clear_mask(IRQ4);
  outb(COM1 + 1, INT_ENABLE_TRANSMIT_EMPTY);

  outb(COM1 + 4, 0x0F);
  printk("serial chip initialized\n");
}
