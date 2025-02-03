MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
.SHELLFLAGS := -eu -c

PLATFORM    = QEMU
QEMU_PATH   = /home/ryan/lab/qemu/build/
TOOLCHAIN   = arm-none-eabi
BUILD_DIR   = build
OUTPUT_ELF  = $(BUILD_DIR)/kernel.elf
OUTPUT_BIN  = $(BUILD_DIR)/kernel.bin
OUTPUT_IMG  = $(BUILD_DIR)/sdcard.img
OUTPUT_DIS  = $(BUILD_DIR)/disassembly.txt

# Toolchain executables
CC          = $(TOOLCHAIN)-gcc
LD          = $(TOOLCHAIN)-ld
OBJCOPY     = $(TOOLCHAIN)-objcopy
MKDIR       = mkdir -p
RM          = rm -rf

# Directories
TEAM_REPO       = ../team-repo
SRC_DIR         = src
BASE_INCLUDE    = $(SRC_DIR)/include
DRIVERS_DIR     = $(SRC_DIR)/drivers/qemu
DRIVERS_BASE    = drivers

# Set flags
CFLAGS = -Wall -Wextra -Wpedantic \
         -mcpu=cortex-a8 -marm \
         -ffreestanding -nostdlib \
         -fno-builtin -fno-common \
         -ffunction-sections -fdata-sections \
         -std=gnu99 -g \
         -MMD -MP

# Optimization flags
# CFLAGS += -O2
# CFLAGS += -fstack-protector-strong -fno-stack-protector

CFLAGS += -I$(TEAM_REPO) -I$(TEAM_REPO)/$(SRC_DIR) -I$(TEAM_REPO)/$(BASE_INCLUDE) -I$(BASE_INCLUDE)



LDFLAGS = -T linker.ld
# LDFLAGS += --gc-sections --print-memory-usage
# Discover all source files

# My kernel code
KERNEL_SRCS 	   = $(wildcard $(SRC_DIR)/*.c)
DRIVER_SRCS        = $(wildcard $(DRIVERS_DIR)/*.c)
BASE_DRIVER_SRCS   = $(wildcard $(SRC_DIR)/$(DRIVERS_BASE)/*.c)

KERNEL_OBJS 	   = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SRCS))
DRIVER_OBJS        = $(patsubst $(DRIVERS_DIR)/%.c,$(BUILD_DIR)/drivers-$(PLATFORM)/%.o,$(DRIVER_SRCS))
BASE_DRIVER_OBJS   = $(patsubst $(SRC_DIR)/$(DRIVERS_BASE)/%.c,$(BUILD_DIR)/drivers/%.o,$(BASE_DRIVER_SRCS))

# Git version
GIT_VERSION = $(shell git describe --always --dirty)

# Other repo code to compile and link against
TEAM_CORE_SRCS          = $(wildcard $(TEAM_REPO)/$(SRC_DIR)/*.c)
TEAM_DRIVER_SRCS        = $(wildcard $(TEAM_REPO)/$(DRIVERS_DIR)/*.c)
TEAM_BASE_DRIVER_SRCS   = $(wildcard $(TEAM_REPO)/$(SRC_DIR)/$(DRIVERS_BASE)/*.c)

# Team objects
TEAM_CORE_OBJS          = $(patsubst $(TEAM_REPO)/$(SRC_DIR)/%.c,$(BUILD_DIR)/team/core/%.o,$(TEAM_CORE_SRCS))
TEAM_DRIVER_OBJS        = $(patsubst $(TEAM_REPO)/$(DRIVERS_DIR)/%.c,$(BUILD_DIR)/team/drivers-$(PLATFORM)/%.o,$(TEAM_DRIVER_SRCS))
TEAM_BASE_DRIVER_OBJS   = $(patsubst $(TEAM_REPO)/$(SRC_DIR)/$(DRIVERS_BASE)/%.c,$(BUILD_DIR)/team/drivers/%.o,$(TEAM_BASE_DRIVER_SRCS))

# ALL team repo objects
ALL_OBJS = $(KERNEL_OBJS) $(DRIVER_OBJS) $(BASE_DRIVER_OBJS) $(TEAM_CORE_OBJS) $(TEAM_DRIVER_OBJS) $(TEAM_BASE_DRIVER_OBJS)
DEP_FILES = $(ALL_OBJS:.o=.d)

# Define build rules
.PHONY: all clean qemu-run disassemble

all: $(OUTPUT_BIN) $(OUTPUT_IMG) disassemble

# Main build targets
$(OUTPUT_ELF): $(ALL_OBJS) | $(BUILD_DIR)
	@echo "[LD] Linking $@"
	$(LD) $(LDFLAGS) -o $@ $^

$(OUTPUT_BIN): $(OUTPUT_ELF)
	@echo "[OBJCOPY] Creating binary $@"
	$(OBJCOPY) -O binary $< $@

$(OUTPUT_IMG): $(OUTPUT_BIN)
	@echo "[IMG] Creating disk image"
	@$(MKDIR) $(@D)
	qemu-img create -f raw $@ 64M
	mkfs.fat -F 32 $@
	mcopy -i $@ $< ::/

# Compilation rules
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC] Kernel: $<"
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/team/core/%.o: $(TEAM_REPO)/$(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC] Team Core: $<"
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/team/drivers-$(PLATFORM)/%.o: $(TEAM_REPO)/$(DRIVERS_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC] Platform Drivers: $<"
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/team/drivers/%.o: $(TEAM_REPO)/$(SRC_DIR)/$(DRIVERS_BASE)/%.c | $(BUILD_DIR)
	@echo "[CC] Base Drivers: $<"
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Build directory creation
$(BUILD_DIR):
	@$(MKDIR) $@

# Dependency inclusion
-include $(DEP_FILES)



$(TEAM_REPO):
	$(MAKE) -C $(TEAM_REPO)


$(OUTPUT_DIS): $(OUTPUT_ELF)
	@echo "[DISASSEMBLY] Generating $(OUTPUT_DIS)"
	$(TOOLCHAIN)-objdump -d $< > $(OUTPUT_DIS)

disassemble: $(OUTPUT_DIS)

# Clean up generated files
clean:
	@echo "[CLEAN] Removing build artifacts"
	$(RM) $(BUILD_DIR)

qemu-run: $(OUTPUT_IMG)
	$(MAKE) -C $(TEAM_REPO) qemu-run QEMU_PATH=$(QEMU_PATH) QEMU_SD_IMG=$(realpath $(OUTPUT_IMG)) PLATFORM=QEMU

.PHONY: all clean qemu-run
