#include "ps2.h"
#include "exit.h"
#include "interrupts.h"
#include "keycodes.h"
#include "portio.h"
#include "printk.h"
#include "processes.h"
#include "ring_buffer.h"
#include "vga.h"

#include <stddef.h>
#include <stdint.h>

static struct RingBuffer keyboard_ring;

struct Ps2Status ps2_get_status() {
  uint8_t status = inb(PS2_STATUS_COMMAND_PORT);
  return *(struct Ps2Status *)&status;
}

struct Ps2ControllerConfig ps2_get_controller_config() {
  ps2_send_command(PS2_COMMAND_READ_BYTE_ZERO);
  uint8_t config = ps2_read_data_block();
  return *(struct Ps2ControllerConfig *)&config;
}

void ps2_send_command(uint8_t command) {
  struct Ps2Status status = ps2_get_status();
  while (status.input_status) {
    status = ps2_get_status();
  }
  outb(PS2_STATUS_COMMAND_PORT, command);
}

void ps2_send_data(uint8_t data) {
  struct Ps2Status status = ps2_get_status();
  while (status.input_status) {
    status = ps2_get_status();
  }
  outb(PS2_DATA_PORT, data);
}

bool ps2_read_data(uint8_t *data, int timeout) {
  struct Ps2Status status = ps2_get_status();
  while (!status.output_status && timeout != 0) {
    status = ps2_get_status();
    if (timeout > 0) {
      timeout -= 1;
    }
  }
  if (timeout != 0) {
    uint8_t frame_data = inb(PS2_DATA_PORT);
    if (data) {
      *data = frame_data;
    }
    return true;
  } else {
    return false;
  }
}

uint8_t ps2_read_data_block() {
  uint8_t data;
  ps2_read_data(&data, PS2_READ_DATA_NO_TIMEOUT);
  return data;
}

void ps2_irq_handler(int num, int error_code, void *arg) {
  enum KeyCode code = ps2_read_data_block();
  if (code == KEYCODE_ENTER_PRESSED) {
    ring_producer_add_char(&keyboard_ring, '\n');
  } else {
    char c = scan_code_to_char(code);
    if (c != '\0') {
      ring_producer_add_char(&keyboard_ring, c);
    }
  }
  PROC_unblock_all(keyboard_ring.blocked);

  PIC_sendEOI(num);
}

void ps2_initialize() {
  dprintk("Initializing keyboard...\n");

  // disable ports
  ps2_send_command(PS2_COMMAND_DISABLE_FIRST_PORT);
  ps2_send_command(PS2_COMMAND_DISABLE_SECOND_PORT);
  // flush output buffer
  struct Ps2Status status = ps2_get_status();
  if (status.output_status) {
    (void)ps2_read_data_block();
  }

  dprintk("Set controller config...\n");

  // set config
  struct Ps2ControllerConfig config = ps2_get_controller_config();
  config.fist_port_interrupt = false;
  config.first_port_translation = false;
  config.first_port_clock = !true; // false is enabled for this one
  ps2_send_command(PS2_COMMAND_WRITE_BYTE_ZERO);
  ps2_send_data(*(uint8_t *)&config);

  dprintk("Run Self Tests...\n");

  // run tests
  ps2_send_command(PS2_COMMAND_SELF_TEST);
  if (ps2_read_data_block() != PS2_SELF_TEST_PASS) {
    dprintk("PS2 Controller Self Test Failure. Stopping.\n");
    EXIT;
  }
  ps2_send_command(PS2_COMMAND_TEST_FIRST_PORT);
  if (ps2_read_data_block() != PS2_INTERFACE_PORT_PASS) {
    dprintk("PS2 Port Test Failure. Stopping.\n");
    EXIT;
  }

  dprintk("Reset Device...\n");

  ps2_send_data(PS2_DEVICE_RESET);
  bool bytes_left = true;
  size_t reset_len = 0;
  uint8_t reset_seq[4];
  const size_t max_reset_seq_len = sizeof(reset_seq) / sizeof(*reset_seq);
  while (bytes_left && reset_len < max_reset_seq_len) {
    bytes_left =
        ps2_read_data(&reset_seq[reset_len], PS2_READ_DATA_RESONABLE_TIMEOUT);
    reset_len += 1;
  }

  if ((reset_seq[0] != PS2_DEVICE_ACK ||
       reset_seq[1] != PS2_DEVICE_RESET_SUCCESS) &&
      (reset_seq[0] != PS2_DEVICE_RESET_SUCCESS ||
       reset_seq[1] != PS2_DEVICE_ACK)) {
    dprintk("PS2 Reset failure\n");
    EXIT;
  }

  ps2_send_data(PS2_DEVICE_GET_SET_SCAN_CODE_SET);
  uint8_t scan = ps2_read_data_block();
  if (scan != PS2_DEVICE_ACK && scan != PS2_DEVICE_RESEND) {
    dprintk("Failed to request setting scan code.\n");
    EXIT;
  }
  ps2_send_data(0x01);
  if (ps2_read_data_block() != PS2_DEVICE_ACK) {
    dprintk("Failed to set scan set.\n");
    EXIT;
  }

  dprintk("Keyboard Initialized.\n");

  dprintk("Enable Interrupts...\n");
  // setup the ring buffer for the handler to write to
  ring_init(&keyboard_ring, true);
  // setup the handler before actually enabling them
  IRQ_handler_set(PS2_INTERRUPT_NUM, ps2_irq_handler, NULL);
  IRQ_clear_mask(IRQ1);
  // actually enable interrupts
  config = ps2_get_controller_config();
  config.fist_port_interrupt = true;
  ps2_send_command(PS2_COMMAND_WRITE_BYTE_ZERO);
  ps2_send_data(*(uint8_t *)&config);

  dprintk("Enable Port...\n");

  ps2_send_command(PS2_COMMAND_ENABLE_FIRST_PORT);
}

void ps2_echo() {
  enum KeyCode code = ps2_read_data_block();
  if (code == KEYCODE_ENTER_PRESSED) {
    printk("\n");
  } else {
    char c = scan_code_to_char(code);
    if (c != '\0') {
      printk("%c", c);
    }
  }
}

int getc() {
  char c;
  ring_consumer_block_next(&keyboard_ring, &c);
  return c;
}
