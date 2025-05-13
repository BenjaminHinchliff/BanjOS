cmake_minimum_required(VERSION 3.30...4.0)

project(
  BanjOS
  VERSION 0.0.1
  LANGUAGES C ASM_NASM)

add_subdirectory("src")

add_custom_command(OUTPUT base_image.img
  COMMAND sudo "${PROJECT_SOURCE_DIR}/tools/mkimg.sh" "${PROJECT_BINARY_DIR}/base_image.img")

add_custom_target(image
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/res/" "${PROJECT_BINARY_DIR}/image/"
  COMMAND sudo "${PROJECT_SOURCE_DIR}/tools/kerncpy.sh" "${PROJECT_BINARY_DIR}/image.img" "${PROJECT_BINARY_DIR}/base_image.img" "${PROJECT_BINARY_DIR}/image/"
  DEPENDS base_image.img
  BYPRODUCTS image.img)
add_dependencies(image kernel)

add_custom_target(run
  COMMAND qemu-system-x86_64 -s -drive format=raw,file=image.img -serial stdio)
add_dependencies(run image)
