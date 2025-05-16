set(INCLUDE_PREFIX "${PROJECT_SOURCE_DIR}/include/")
set(INCLUDES
  "gdt.h"
  "interrupts.h"
  "keycodes.h"
  "portio.h"
  "printk.h"
  "ps2.h"
  "serial.h"
  "vga.h"
  "multiboot_tags.h"
  "smolassert.h"
  "exit.h"
  "allocator.h")
list(TRANSFORM INCLUDES PREPEND ${INCLUDE_PREFIX})

set(SRCS
  "gdt.c"
  "interrupts.c"
  "keycodes.c"
  "kmain.c"
  "printk.c"
  "ps2.c"
  "serial.c"
  "string.c"
  "vga.c"
  "multiboot_tags.c"
  "allocator.c")

set(ASMS
  "boot.asm"
  "isr_handler.asm"
  "long_mode_init.asm"
  "multiboot_header.asm")

set(KERNEL_TARGET "kernel")
set(KERNEL_OUTPUT "${CMAKE_BINARY_DIR}/image/boot")
add_executable(${KERNEL_TARGET} ${SRCS} ${ASMS} ${INCLUDES})
target_include_directories(${KERNEL_TARGET} PRIVATE ${INCLUDE_PREFIX})

target_compile_definitions(${KERNEL_TARGET} PRIVATE MMU_MEMTEST)

file(MAKE_DIRECTORY ${KERNEL_OUTPUT})
set_target_properties(${KERNEL_TARGET} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${KERNEL_OUTPUT})
