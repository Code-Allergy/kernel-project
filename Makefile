# Master makefile for CMPT432 solo project
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-print-directory
.SHELLFLAGS := -eu -c

# Target arch
PLATFORM    := QEMU
ARCH		:= arm
CPU			:= cortex-a8

TOOLCHAIN   = arm-none-eabi
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  := $(MAKEFILE_DIR)build
BUILD_DIR 	:= $(BUILD_BASE)/$(PLATFORM)
BUILD_DIR   ?= $(O)

# SD card image options
SDCARD_IMAGE_NAME = sdcard.img
SDCARD_BUILD_DIR = $(BUILD_DIR)
SDCARD_IMAGE_SIZE = 64M
SDCARD_KERNEL_PATH = /boot/kernel.bin
SDCARD_USERSPACE_BIN = /bin
SDCARD_USERSPACE_ELF = /elf

# Directories
TOOLS_DIR = tools
KERNEL_DIR = kernel
BOOTLOADER_DIR = bootloader
USERSPACE_DIR = userspace
BOOTLOADER_BUILD = $(BUILD_DIR)/bootloader
USERSPACE_BUILD = $(BUILD_DIR)/userspace


# Output files
OUTPUT_ELF  = $(BUILD_DIR)/kernel.elf
OUTPUT_BIN  = $(BUILD_DIR)/kernel.bin
OUTPUT_IMG  = $(BUILD_DIR)/sdcard.img

OUTPUT_USER_ELF = $(BUILD_DIR)/userspace.elf

BOOTLOADER_BIN = $(BOOTLOADER_BUILD)/bootloader.bin

# Commands
RM		  = rm -rf

.PHONY: kernel clean sdcard bootloader qemu userspace boot-bbb

all: sdcard


sdcard: $(SDCARD_BUILD_DIR)/$(SDCARD_IMAGE_NAME)

$(SDCARD_BUILD_DIR)/$(SDCARD_IMAGE_NAME): kernel bootloader userspace
	@echo "[SDCARD]  Creating SD card image"
	@qemu-img create -f raw $@ $(SDCARD_IMAGE_SIZE) 1> /dev/null
	@mkfs.fat -F 32 $@ 1> /dev/null
	@mmd -i $@ ::/boot
	@mmd -i $@ ::/bin
	@mmd -i $@ ::/elf
	@echo "Hello World!" > $(BUILD_DIR)/hello.txt
	@mcopy -i $@ $(BUILD_DIR)/hello.txt ::/hello.txt
	@mcopy -i $@ $(OUTPUT_BIN) ::$(SDCARD_KERNEL_PATH)
	@mcopy -i $@ $(USERSPACE_BUILD)/bin/* ::$(SDCARD_USERSPACE_BIN)
	@mcopy -i $@ $(USERSPACE_BUILD)/elf/* ::$(SDCARD_USERSPACE_ELF)
	@echo "[OUTPUT]   SD card image created at $@"

kernel:
	@echo "[MAKE]    Building kernel"
	@$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=$(BUILD_DIR) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CPU=$(CPU)

bootloader:
	@echo "[MAKE]    Building bootloader"
	@$(MAKE) -C $(BOOTLOADER_DIR) BUILD_DIR=$(BOOTLOADER_BUILD) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CPU=$(CPU)

userspace:
	@echo "[MAKE]    Building userspace"
	@$(MAKE) -C $(USERSPACE_DIR) BUILD_DIR=$(USERSPACE_BUILD) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CPU=$(CPU)

qemu: sdcard
	$(QEMU_PATH)qemu-system-arm -m 512M -M cubieboard \
	-cpu cortex-a8 -drive if=sd,format=raw,file=$(OUTPUT_IMG) \
	-serial mon:stdio -nographic \
	-kernel $(BOOTLOADER_BIN) \
	-d guest_errors,unimp,int  -D qemu.log


qemu-gdb: sdcard
	$(QEMU_PATH)qemu-system-arm -m 512M -M cubieboard \
	-cpu cortex-a8 -drive if=sd,format=raw,file=$(OUTPUT_IMG) \
	-serial mon:stdio -nographic \
	-d guest_errors,unimp,int \
	-kernel $(BOOTLOADER_BIN) -s -S

bbb: #kernel
	@$(MAKE) -C $(BOOTLOADER_DIR) PLATFORM=BBB BUILD_DIR=$(BUILD_BASE)/BBB ARCH=$(ARCH) CPU=$(CPU)


flash-bbb: bbb
	$(TOOLS_DIR)/sdimager/flash_img.sh $(BUILD_BASE)/BBB/bootloader.img $(DEV)

boot-bbb: bbb
	$(TOOLS_DIR)/boot_uart.sh $(BUILD_BASE)/BBB/bootloader.img

CLEAN_LABEL = [\033[0;32mCLEAN\033[0m]
clean:
	@echo "[CLEAN]   Cleaning project build directory"
	$(RM) $(BUILD_BASE)
