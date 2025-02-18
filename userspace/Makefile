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
CFLAGS = -O0 -Wall -mcpu=cortex-a8 -marm -nostdlib -ffreestanding -I.
LDFLAGS = -T linker.ld -Llib -L$(GCC_LIB_DIR)
LDLIBS = -losc -lgcc

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
BINARY_TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SOURCES))

all: $(BINARY_TARGETS)

# Rule to create binary for each source file
$(BIN_DIR)/%: $(ELF_DIR)/%.elf | $(BIN_DIR)
	$(OBJCOPY) -O binary $< $@

# Create elf from object files
$(ELF_DIR)/%.elf: $(OBJ_DIR)/%.o | $(ELF_DIR)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile C source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) make-lib
	$(CC) $(CFLAGS) -c $< -o $@

# Create necessary directories if they don't exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(ELF_DIR):
	@mkdir -p $(ELF_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

make-lib:
	$(MAKE) -C lib

# Clean target to remove generated files
clean:
	$(MAKE) -C lib clean
	rm -rf $(BUILD_DIR)

.PRECIOUS: $(ELF_DIR)/%.elf $(OBJ_DIR)/%.o
.PHONY: all clean make-lib
