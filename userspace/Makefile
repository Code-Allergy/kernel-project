# Makefile for compiling the userspace programs
BUILD_DIR := build
SRC_DIR := src
ELF_DIR := $(BUILD_DIR)/elf
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

# Compiler and linker
TOOLCHAIN_PREFIX = arm-none-eabi-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy

GCC_LIB_DIR = $(dir $(shell $(CC) -print-libgcc-file-name))


# Compiler flags
CFLAGS = -g -O0 -Wall -mcpu=cortex-a8 -marm -nostdlib -nostdinc -ffreestanding -I. -Ilib/include

# LDFLAGS = -Llib -L$(GCC_LIB_DIR) --entry _start
LDFLAGS = -T linker.ld -Llib -L$(GCC_LIB_DIR)

# for now, we load everything in one region
LDFLAGS += --no-warn-rwx-segments

LDLIBS = -losc -lgcc


SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
BINARY_TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SOURCES))

all: $(BINARY_TARGETS)

# Rule to create binary for each source file
$(BIN_DIR)/%: $(ELF_DIR)/%.elf | $(BIN_DIR)
	@echo "[OBJCOPY] Creating stripped binary $@"
	@$(OBJCOPY) -O binary $< $@

# Create elf from object files
$(ELF_DIR)/%.elf: $(OBJ_DIR)/%.o | $(ELF_DIR)
	@echo "[LD]      Linking userspace program $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile C source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) make-lib
	@echo "[CC]      Compiling userspace program $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Create necessary directories if they don't exist
$(BUILD_DIR):
	@echo "[MKDIR]   Creating directory $(BUILD_DIR)"
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@echo "[MKDIR]   Creating directory $(BIN_DIR)"
	@mkdir -p $(BIN_DIR)

$(ELF_DIR):
	@echo "[MKDIR]   Creating directory $(ELF_DIR)"
	@mkdir -p $(ELF_DIR)

$(OBJ_DIR):
	@echo "[MKDIR]   Creating directory $(OBJ_DIR)"
	@mkdir -p $(OBJ_DIR)

make-lib:
	@echo "[MAKE]    Building lib"
	@$(MAKE) -C lib

# Clean target to remove generated files
clean:
	@echo "[CLEAN]   Cleaning up userspace programs"
	@$(MAKE) -C lib clean
	@rm -rf $(BUILD_DIR)

.PRECIOUS: $(ELF_DIR)/%.elf $(OBJ_DIR)/%.o
.PHONY: all clean make-lib
