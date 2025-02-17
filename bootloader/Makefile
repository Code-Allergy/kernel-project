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
LDFLAGS = -T $(LDSCRIPT) --build-id=none -nostdlib -static --gc-sections
OBJCOPY = arm-none-eabi-objcopy
CC = arm-none-eabi-gcc

# ======================================================================
# Build Configuration
# ======================================================================
IFLAGS = -I. -I../kernel/src -I../include
CCDEFINES = -DPLATFORM=$(PLATFORM) -DPLATFORM_$(PLATFORM) -DBOOTLOADER
CFLAGS = -Wall -Wextra -g -mcpu=cortex-a8 -mfloat-abi=soft -pedantic $(CCDEFINES)
CFLAGS += -static -ffreestanding -fbuiltin -marm -MMD -MP


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

$(OUTPUT_MLO): $(OUTPUT_BIN)
	$(SD_IMAGER_DIR)/mk-gpimage $(MLO_DEST_ADDR) $< $@

$(OUTPUT_SDIMG): $(OUTPUT_MLO)
	cp $(SD_IMAGER_DIR)/raw-mmc-header.img $@
	dd if=$< of=$@ iflag=fullblock conv=sync seek=1 status=none
	echo 'label: dos' | sfdisk --quiet $@

$(OUTPUT_ELF): $(ALL_OBJS)
	echo "Linking $@ $^"
	echo "Core objs $(CORE_OBJS)"
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "Build complete for platform: $(PLATFORM)"

$(OUTPUT_BIN): $(OUTPUT_ELF)
	$(OBJCOPY) -O binary $< $@

# ======================================================================
# Compilation Rules
# ======================================================================
# Bootloader assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@mkdir -p $(@D)
	@echo "Compiling bootloader: $<"
	$(CC) $(IFLAGS) $(CFLAGS) -E $< -o $(BUILD_DIR)/$*.i
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$*.i -o $@

# Bootloader C entry file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(@D)
	@echo "Compiling bootloader: $<"
	$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

# Core kernel C files
$(BUILD_DIR)/kernel/%.o: $(KERNEL_SRC_DIR)/%.c | $(BUILD_DIR)/kernel
	@mkdir -p $(@D)
	@echo "Compiling core: $<"
	$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

# Platform-specific drivers
$(BUILD_DIR)/drivers-$(PLATFORM)/%.o: $(DRIVERS_DIR)/%.c | $(BUILD_DIR)/drivers-$(PLATFORM)
	@mkdir -p $(@D)
	@echo "Compiling driver: $<"
	$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: $(DRIVERS_BASE)/%.c | $(BUILD_DIR)/drivers
	@mkdir -p $(@D)
	@echo "Compiling general driver: $<"
	$(CC) $(IFLAGS) $(CFLAGS) -c $< -o $@

# ======================================================================
# Make directories
# ======================================================================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/kernel:
	mkdir -p $(BUILD_DIR)/kernel

$(BUILD_DIR)/drivers:
	mkdir -p $(BUILD_DIR)/drivers

$(BUILD_DIR)/drivers-$(PLATFORM):
	mkdir -p $(BUILD_DIR)/drivers-$(PLATFORM)

-include $(DEP_FILES)
# ======================================================================
# Utilities
# ======================================================================
clean:
	@rm -rf $(BUILD_DIR) $(OUTPUT_ELF) $(OUTPUT_BIN) disassembly.txt


.PHONY: all clean
