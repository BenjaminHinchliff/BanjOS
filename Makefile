ifneq (,$(wildcard ./.env))
    include .env
    export
endif

AS := nasm
CC := $(CROSS_DIR)/bin/x86_64-elf-gcc
LD := $(CROSS_DIR)/bin/x86_64-elf-ld

KERNEL_EXEC := boot/kernel
LINKER_SCRIPT := linker.ld
IMAGE_NAME := image.img

BUILD_DIR := ./build
SRC_DIRS := ./src
IMAGE_DIR := ./image
RES_DIR := ./res

RESS := $(shell find $(RES_DIR) -type f)

SRCS := $(shell find $(SRC_DIRS) -name '*.c' -or -name '*.asm')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

IMAGE_RESS := $(RESS:$(RES_DIR)/%=$(IMAGE_DIR)/%)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CFLAGS := $(INC_FLAGS) -I/usr/include -MMD -MP -O3 -g -mno-red-zone -Wall -std=gnu99
ASFLAGS := -felf64
LDFLAGS := -n

.PHONY: run clean image build

build: $(IMAGE_DIR)/$(KERNEL_EXEC) $(IMAGE_RESS)

image: build $(IMAGE_NAME)

run: image
	qemu-system-x86_64 -s -drive format=raw,file=$(IMAGE_NAME) -serial stdio

clean:
	rm -rf $(BUILD_DIR) $(IMAGE_DIR) $(IMAGE_NAME)

$(IMAGE_NAME): build
	sudo ./mkimg.sh $(IMAGE_NAME) $(IMAGE_DIR)

$(IMAGE_DIR)/$(KERNEL_EXEC): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $@ $(OBJS)

$(IMAGE_DIR)/%: $(RES_DIR)/%
	@mkdir -p $(dir $@)
	cp $< $@

# Build step for asm source
$(BUILD_DIR)/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)
