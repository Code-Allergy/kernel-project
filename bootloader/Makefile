# Default platform is QEMU
PLATFORM ?=QEMU
DRIVERS_BASE = ../drivers

# Set platform-specific configurations
ifeq ($(PLATFORM), BBB)
	LDSCRIPT = ldscripts/bbb.ld
	DRIVERS_DIR = $(DRIVERS_BASE)/bbb
else
	LDSCRIPT = ldscripts/qemu-cubieboard.ld
	DRIVERS_DIR = $(DRIVERS_BASE)/qemu
endif

# ======================================================================
# Toolchain Configuration
# ======================================================================
AS = arm-none-eabi-as
ASFLAGS = -mcpu=cortex-a8 -g
LD = arm-none-eabi-ld
LGCC := $(shell arm-none-eabi-gcc -print-libgcc-file-name | xargs dirname)
LDFLAGS = -T $(LDSCRIPT) --build-id=none -nostdlib -static --gc-sections -L$(LGCC)
OBJCOPY = arm-none-eabi-objcopy
CC = arm-none-eabi-gcc

# ======================================================================
# Build Configuration
# ======================================================================
IFLAGS = -I. -I../kernel/src -I../include
CCDEFINES = -DPLATFORM=$(PLATFORM) -DPLATFORM_$(PLATFORM) -DBOOTLOADER
CFLAGS = -Wall -Wextra -g -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=soft -pedantic $(CCDEFINES)
CFLAGS += -static -ffreestanding -fbuiltin -marm -MMD -MP -Wno-builtin-declaration-mismatch

CFLAGS += -Wno-unused-parameter

LDLIBS = -lgcc

# ======================================================================
# File Paths
# ======================================================================
OUTPUT_ELF = $(BUILD_DIR)/bootloader.elf
OUTPUT_BIN = $(BUILD_DIR)/bootloader.bin
OUTPUT_SDIMG = $(BUILD_DIR)/bootloader.img
OUTPUT_MLO = $(BUILD_DIR)/MLO

MLO_DEST_ADDR = 0x402f0400

BUILD_DIR = build/$(PLATFORM)
SRC_DIR = src
KERNEL_DIR = ../kernel
SD_IMAGER_DIR = ../tools/sdimager

# Full paths
KERNEL_SRC_DIR = $(KERNEL_DIR)/$(SRC_DIR)

# Collect source files
BOOT_SRCS = $(SRC_DIR)/start.S $(SRC_DIR)/init.S
BOOT_CSRCS = $(SRC_DIR)/bootloader.c
CORE_SRCS = $(wildcard $(KERNEL_SRC_DIR)/*.c)
DRIVER_SRCS = $(wildcard $(DRIVERS_DIR)/*.c)
BASE_DRIVER_SRCS = $(wildcard $(DRIVERS_BASE)/*.c)

# # Generate object file paths
BOOT_OBJS = $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(BOOT_SRCS))
BOOT_CSRCS_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(BOOT_CSRCS))
CORE_OBJS = $(patsubst $(KERNEL_SRC_DIR)/%.c,$(BUILD_DIR)/kernel/%.o,$(CORE_SRCS))
DRIVER_OBJS = $(patsubst $(DRIVERS_DIR)/%.c,$(BUILD_DIR)/drivers-$(PLATFORM)/%.o,$(DRIVER_SRCS))
BASE_DRIVER_OBJS = $(patsubst $(DRIVERS_BASE)/%.c,$(BUILD_DIR)/drivers/%.o,$(BASE_DRIVER_SRCS))

ALL_OBJS = $(BOOT_OBJS) $(CORE_OBJS) $(DRIVER_OBJS) $(BASE_DRIVER_OBJS) $(BOOT_CSRCS_OBJS)
DEP_FILES = $(ALL_OBJS:.o=.d)
# TOP_DIR = ..


# ======================================================================
# Build Targets
# ======================================================================
all: $(OUTPUT_ELF) $(OUTPUT_BIN) $(OUTPUT_SDIMG) $(OUTPUT_MLO)


$(OUTPUT_SDIMG): $(OUTPUT_MLO)
	@echo "[IMG]     Creating SD card image"
	@cp $(SD_IMAGER_DIR)/raw-mmc-header.img $@
	@dd if=$< of=$@ iflag=fullblock conv=sync seek=1 status=none
	@echo 'label: dos' | sfdisk --quiet $@

$(OUTPUT_MLO): $(OUTPUT_BIN)
	@echo "[MLO]     Creating MLO image"
	@$(SD_IMAGER_DIR)/mk-gpimage $(MLO_DEST_ADDR) $< $@

$(OUTPUT_ELF): $(ALL_OBJS)
	@echo "[LD]      Linking $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUTPUT_BIN): $(OUTPUT_ELF)
	@echo "[OC]      Converting $@ to binary"
	@$(OBJCOPY) -O binary $< $@

# ======================================================================
# Compilation Rules
# ======================================================================
# Bootloader assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@mkdir -p $(@D)
	@echo "[ASM]     Bootloader assembly: $<"
	@$(CC) $(IFLAGS) $(CFLAGS) -E $< -o $(BUILD_DIR)/$*.i
	@$(AS) $(ASFLAGS) $(BUILD_DIR)/$*.i -o $@

# Bootloader C entry file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(@D)
	@echo "[CC]      Bootloader C: $<"
	@$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

# Core kernel C files
$(BUILD_DIR)/kernel/%.o: $(KERNEL_SRC_DIR)/%.c | $(BUILD_DIR)/kernel
	@mkdir -p $(@D)
	@echo "[CC]      Bootloader kernel core: $<"
	@$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

# Platform-specific drivers
$(BUILD_DIR)/drivers-$(PLATFORM)/%.o: $(DRIVERS_DIR)/%.c | $(BUILD_DIR)/drivers-$(PLATFORM)
	@mkdir -p $(@D)
	@echo "[CC]      Bootloader platform driver: $<"
	@$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: $(DRIVERS_BASE)/%.c | $(BUILD_DIR)/drivers
	@mkdir -p $(@D)
	@echo "[CC]      Bootloader general driver: $<"
	@$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

# ======================================================================
# Make directories
# ======================================================================
$(BUILD_DIR):
	@echo "[MKDIR]   Creating build directory"
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/kernel:
	@echo "[MKDIR]   Creating kernel build directory"
	@mkdir -p $(BUILD_DIR)/kernel

$(BUILD_DIR)/drivers:
	@echo "[MKDIR]   Creating drivers build directory"
	@mkdir -p $(BUILD_DIR)/drivers

$(BUILD_DIR)/drivers-$(PLATFORM):
	@echo "[MKDIR]   Creating platform drivers build directory"
	@mkdir -p $(BUILD_DIR)/drivers-$(PLATFORM)

-include $(DEP_FILES)
# ======================================================================
# Utilities
# ======================================================================
clean:
	@echo "[CLEAN]   Cleaning up"
	@rm -rf $(BUILD_DIR) $(OUTPUT_ELF) $(OUTPUT_BIN) disassembly.txt


.PHONY: all clean
