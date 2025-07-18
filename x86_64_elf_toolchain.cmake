set(CROSS_DIR "$ENV{HOME}/Code/CSC454/cross/dist")

set(CMAKE_C_COMPILER "${CROSS_DIR}/bin/x86_64-elf-gcc")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/include")
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_LINKER "${CROSS_DIR}/bin/x86_64-elf-ld")
set(CMAKE_C_LINK_FLAGS "-n -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_C_LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/image/")
