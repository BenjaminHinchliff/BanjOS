cmake_minimum_required(VERSION 3.30...4.0)

project(
  BanjOS
  VERSION 0.0.1
  LANGUAGES C ASM_NASM)

add_subdirectory("src")

add_custom_target(run
  COMMAND qemu-system-x86_64 -s -drive format=raw,file=image.img -serial stdio)
add_dependencies(run image)
