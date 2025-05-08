#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_COMMAND_PORT 0x64

#define PS2_COMMAND_ENABLE_FIRST_PORT 0xAE
#define PS2_COMMAND_ENABLE_SECOND_PORT 0xA8
#define PS2_COMMAND_DISABLE_FIRST_PORT 0xAD
#define PS2_COMMAND_DISABLE_SECOND_PORT 0xA7
#define PS2_COMMAND_READ_BYTE_ZERO 0x20
#define PS2_COMMAND_WRITE_BYTE_ZERO 0x60
#define PS2_COMMAND_SELF_TEST 0xAA
#define PS2_COMMAND_TEST_FIRST_PORT 0xAB
#define PS2_COMMAND_TEST_SECOND_PORT 0xA9

#define PS2_DEVICE_RESET 0xFF
#define PS2_DEVICE_GET_SET_SCAN_CODE_SET 0xF0

#define PS2_DEVICE_ACK 0xFA
#define PS2_DEVICE_RESEND 0xFE
#define PS2_DEVICE_RESET_SUCCESS 0xAA

#define PS2_SELF_TEST_PASS 0x55
#define PS2_INTERFACE_PORT_PASS 0x00

#define PS2_READ_DATA_NO_TIMEOUT -1
#define PS2_READ_DATA_RESONABLE_TIMEOUT 10000

#define PS2_INTERRUPT_NUM 0x21

struct Ps2Status {
  bool output_status : 1;
  bool input_status : 1;
  bool system_flag : 1;
  bool command_or_data : 1;
  uint8_t unknown : 2;
  bool timeout_err : 1;
  bool parity_err : 1;
} __attribute__((packed));

struct Ps2ControllerConfig {
  bool fist_port_interrupt : 1;
  bool second_port_interrupt : 1;
  bool sysflag : 1;
  bool : 1;
  bool first_port_clock : 1;
  bool second_port_clock : 1;
  bool first_port_translation : 1;
  bool : 1;
} __attribute__((packed));

struct Ps2Status ps2_get_status();

struct Ps2ControllerConfig ps2_get_controller_config();

void ps2_send_command(uint8_t command);
void ps2_send_data(uint8_t data);

bool ps2_read_data(uint8_t *data, int timeout);
uint8_t ps2_read_data_block();

bool ps2_initialize();

void ps2_echo();
