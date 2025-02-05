# Master makefile for CMPT432 solo project
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
.SHELLFLAGS := -eu -c

QEMU_PATH   = /home/ryan/lab/qemu/build/
TOOLCHAIN   = arm-none-eabi
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR 	:= $(MAKEFILE_DIR)build
BUILD_DIR   ?= $(O)

# Target arch
PLATFORM    := QEMU
ARCH		:= arm
CPU			:= cortex-a8

# Output files
OUTPUT_ELF  = $(BUILD_DIR)/kernel.elf
OUTPUT_BIN  = $(BUILD_DIR)/kernel.bin
OUTPUT_IMG  = $(BUILD_DIR)/sdcard.img

OUTPUT_USER_ELF = $(BUILD_DIR)/userspace.elf

BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin

# SD card image options
SDCARD_IMAGE_NAME = sdcard.img
SDCARD_BUILD_DIR = $(BUILD_DIR)
SDCARD_IMAGE_SIZE = 64M
SDCARD_KERNEL_PATH = /boot/kernel.bin
SDCARD_USERSPACE_BIN = /bin
SDCARD_USERSPACE_ELF = /elf

# Directories
KERNEL_DIR = kernel
BOOTLOADER_DIR = team-repo
USERSPACE_DIR = userspace
USERSPACE_BUILD = $(BUILD_DIR)/userspace


# Commands
RM		  = rm -rf

.PHONY: kernel clean sdcard bootloader qemu userspace

all: sdcard


sdcard: $(SDCARD_BUILD_DIR)/$(SDCARD_IMAGE_NAME)

$(SDCARD_BUILD_DIR)/$(SDCARD_IMAGE_NAME): kernel bootloader userspace
	@qemu-img create -f raw $@ $(SDCARD_IMAGE_SIZE) 1> /dev/null
	@mkfs.fat -F 32 $@ 1> /dev/null
	@mmd -i $@ ::/boot
	@mmd -i $@ ::/bin
	@mmd -i $@ ::/elf
	@mcopy -i $@ $(OUTPUT_BIN) ::$(SDCARD_KERNEL_PATH)
	@mcopy -i $@ $(USERSPACE_BUILD)/bin/* ::$(SDCARD_USERSPACE_BIN)
	@mcopy -i $@ $(USERSPACE_BUILD)/elf/* ::$(SDCARD_USERSPACE_ELF)
	@echo "SD card image created at $@"

kernel: 
	@$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=$(BUILD_DIR) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CPU=$(CPU) 1> /dev/null

bootloader:
	@$(MAKE) -C $(BOOTLOADER_DIR) BUILD_DIR=$(BUILD_DIR) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CPU=$(CPU) 1> /dev/null

userspace:
	@$(MAKE) -C $(USERSPACE_DIR) BUILD_DIR=$(USERSPACE_BUILD) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CPU=$(CPU) 1> /dev/null

qemu: sdcard
	$(QEMU_PATH)qemu-system-arm -m 512M -M cubieboard \
	-cpu cortex-a8 -drive if=sd,format=raw,file=$(OUTPUT_IMG) \
	-serial mon:stdio -nographic \
	-d guest_errors,mmu,unimp,invalid_mem,page,int \
	-kernel $(BOOTLOADER_BIN)


clean:
	$(RM) $(BUILD_DIR)