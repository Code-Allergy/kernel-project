# MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
.SHELLFLAGS := -eu -c

PLATFORM    = QEMU
TOOLCHAIN   = arm-none-eabi
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR 	:= $(MAKEFILE_DIR)build
OUTPUT_ELF  = $(BUILD_DIR)/kernel.elf
OUTPUT_BIN  = $(BUILD_DIR)/kernel.bin
OUTPUT_IMG  = $(BUILD_DIR)/sdcard.img
OUTPUT_DIS  = $(BUILD_DIR)/disassembly.txt
GIT_VERSION = $(shell git describe --always --dirty)

# Toolchain executables
CC          = $(TOOLCHAIN)-gcc
LD          = $(TOOLCHAIN)-ld
AS		  	= $(TOOLCHAIN)-as
OBJCOPY     = $(TOOLCHAIN)-objcopy
MKDIR       = mkdir -p
RM          = rm -rf

# Directories
SRC_DIR         = src
ASM_DIR         = asm
BASE_INCLUDE    = ../include
DRIVERS_BASE    = ../drivers

# TODO BBB support
DRIVERS_DIR     = $(DRIVERS_BASE)/qemu


LIBGCC_PATH = $(dir $(shell $(CC) -print-libgcc-file-name))

# Set flags
CFLAGS := -Wall -Wextra -Wpedantic \
         -mcpu=cortex-a8 -marm -mfpu=neon \
         -ffreestanding -nostdlib -nostdinc \
         -fno-builtin -fno-common \
         -ffunction-sections -fdata-sections \
         -std=gnu99 \
         -MMD -MP

# Optimization flags
# CFLAGS += -g -O0
# CFLAGS += -fstack-protector-all -fstack-protector-none -O0 -DDEBUG
CFLAGS += -fno-stack-protector -O2

CFLAGS += -I$(BASE_INCLUDE) -I$(SRC_DIR) -I. -I../bootloader

CFLAGS += -DPLATFORM_$(PLATFORM) -DGIT_VERSION=\"$(GIT_VERSION)\"

# for now, ignore unused parameters

ASFLAGS = -mcpu=cortex-a8 -mfpu=neon

LGCC := $(shell arm-none-eabi-gcc -print-libgcc-file-name | xargs dirname)
LDFLAGS = -T kernel.ld -L$(LGCC)
LDFLAGS += --gc-sections

LDLIBS = -lgcc

# My kernel code
KERNEL_SRCS 	   = $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(SRC_DIR)/*.c)
KERNEL_ASM_SRCS    = $(wildcard $(SRC_DIR)/$(ASM_DIR)/*.S)
DRIVER_SRCS        = $(wildcard $(DRIVERS_DIR)/*.c)
BASE_DRIVER_SRCS   = $(wildcard $(DRIVERS_BASE)/*.c)

KERNEL_OBJS 	   = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SRCS))
KERNEL_ASM_OBJS    = $(patsubst $(SRC_DIR)/$(ASM_DIR)/%.S,$(BUILD_DIR)/kernel/$(ASM_DIR)/%.o,$(KERNEL_ASM_SRCS))
DRIVER_OBJS        = $(patsubst $(DRIVERS_DIR)/%.c,$(BUILD_DIR)/drivers-$(PLATFORM)/%.o,$(DRIVER_SRCS))
BASE_DRIVER_OBJS   = $(patsubst $(DRIVERS_BASE)/%.c,$(BUILD_DIR)/drivers/%.o,$(BASE_DRIVER_SRCS))

# ALL team repo objects
ALL_OBJS = $(KERNEL_ASM_OBJS) $(KERNEL_OBJS) $(DRIVER_OBJS) $(BASE_DRIVER_OBJS)
DEP_FILES = $(ALL_OBJS:.o=.d)

# Define build rules
.PHONY: all clean qemu-run disassemble distclean #userspace

all: $(OUTPUT_BIN) $(OUTPUT_IMG)

# Main build targets
$(OUTPUT_ELF): $(ALL_OBJS) | $(BUILD_DIR)
	@echo "[LD]      Linking $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUTPUT_BIN): $(OUTPUT_ELF)
	@echo "[OBJCOPY] Creating binary $@"
	@$(OBJCOPY) -O binary $< $@

$(OUTPUT_IMG): $(OUTPUT_BIN)
	@echo "[IMG]     Creating disk image"
	@$(MKDIR) $(@D)
	@qemu-img create -f raw $@ 64M > /dev/null
	@mkfs.fat -F 32 $@ > /dev/null
	@mmd -i $@ ::/boot
	@mmd -i $@ ::/bin
	@mmd -i $@ ::/elf
	@mcopy -i $@ $< ::/boot/kernel.bin

################################################################################################
# kernel Compilation rules
################################################################################################
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/kernel
	@echo "[CC]      Kernel: $<"
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/$(ASM_DIR)/%.o: $(SRC_DIR)/$(ASM_DIR)/%.S | $(BUILD_DIR)/kernel/$(ASM_DIR)
	@echo "[ASM]     Kernel ASM: $<"
	@$(CC) $(CFLAGS) -E $< -o $(BUILD_DIR)/kernel/$*.i
	@$(AS) $(ASFLAGS) $(BUILD_DIR)/kernel/$*.i -o $@


$(BUILD_DIR)/drivers-$(PLATFORM)/%.o: $(DRIVERS_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC]      Kernel Platform Drivers: $<"
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: $(DRIVERS_BASE)/%.c | $(BUILD_DIR)
	@echo "[CC]      Kernel Base Drivers: $<"
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@


################################################################################################
# Build directory creation
################################################################################################

$(BUILD_DIR):
	@$(MKDIR) $@

$(BUILD_DIR)/kernel:
	@$(MKDIR) $@

$(BUILD_DIR)/kernel/$(ASM_DIR):
	@$(MKDIR) $@

################################################################################################
# Dependency inclusion
################################################################################################
-include $(DEP_FILES)

################################################################################################
# Clean up
################################################################################################
clean:
	@echo "[CLEAN] Removing build artifacts"
	@$(RM) $(BUILD_DIR)
################################################################################################
# Run in QEMU
################################################################################################
qemu-run: $(OUTPUT_IMG)
	$(MAKE) -C $(TEAM_REPO) qemu-run QEMU_PATH=$(QEMU_PATH) QEMU_SD_IMG=$(realpath $(OUTPUT_IMG)) PLATFORM=QEMU

.PHONY: all clean qemu-run
