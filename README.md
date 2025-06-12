# CSC454 OS Project (BanjOS)

Model operating system for the CSC454 course at Cal Poly.

It's a terrible operating system that at best barely works and is full of race
conditions because I am a very amazing and careful programmer. Conforms for
course material up until Ext2 file reading.

## Building

Build is done using cmake, however build requires a custom toolchain to get the
cross compiler working properly. The code currently assumes that the cross
compiler is in the `cross/dist` directory, but this can be configured by
setting the `CROSS_DIR` variable inside the `x86_64_elf_toolchain.cmake` file.

Testing was done on a debug build, effects of optimizations haven't been
tested. I do know that optimizations that make use of the vector or floating
point registers will cause issues as the threading routines currently don't
support saving them.

```shell
cmake --toolchain=x86_64_elf_toolchain.cmake -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug .
```

## Running

To run the operating system in the QEMU emulator, simply use the run target in the generated buildsystem.
```shell
make run
```
