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
  "page_allocator.h"
  "page_table.h"
  "allocator.h"
  "processes.h")
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
  "page_allocator.c"
  "page_table.c"
  "allocator.c"
  "processes.c")

set(ASMS
  "boot.asm"
  "isr_handler.asm"
  "long_mode_init.asm"
  "multiboot_header.asm")

set(KERNEL_TARGET "kernel")
set(KERNEL_OUTPUT "${CMAKE_BINARY_DIR}/image/boot")
add_executable(${KERNEL_TARGET} ${SRCS} ${ASMS} ${INCLUDES})
target_include_directories(${KERNEL_TARGET} PRIVATE ${INCLUDE_PREFIX})

file(MAKE_DIRECTORY ${KERNEL_OUTPUT})
set_target_properties(${KERNEL_TARGET} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${KERNEL_OUTPUT})

add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/base_image.img"
  COMMAND sudo "${PROJECT_SOURCE_DIR}/tools/mkimg.sh" "${PROJECT_BINARY_DIR}/base_image.img")

add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/image.img"
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/res/" "${PROJECT_BINARY_DIR}/image/"
  COMMAND sudo "${PROJECT_SOURCE_DIR}/tools/kerncpy.sh" "${PROJECT_BINARY_DIR}/image.img" "${PROJECT_BINARY_DIR}/base_image.img" "${PROJECT_BINARY_DIR}/image/"
  DEPENDS
    "$<TARGET_FILE:${KERNEL_TARGET}>"
    "${PROJECT_BINARY_DIR}/base_image.img")

add_custom_target(image DEPENDS "${PROJECT_BINARY_DIR}/image.img")
