# Makefile for compiling the userspace programs
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

# Compiler and linker
TOOLCHAIN_PREFIX = arm-none-eabi-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy

# Compiler flags; adjust as needed (for assembly you might not need many flags)
CFLAGS = -O2 -Wall -mcpu=cortex-a8 -marm -nostdlib -ffreestanding

LDFLAGS = -T linker.ld

# Target binary name
ELF_TARGET = $(BUILD_DIR)/tester.elf
TARGET = $(BIN_DIR)/tester

# Source file
SOURCES = tester.S
OBJECTS = $(OBJ_DIR)/tester.o

# Default target: build the binary
all: $(ELF_TARGET) $(TARGET)

# Build the binary
$(TARGET): $(ELF_TARGET) | $(BIN_DIR)
	arm-none-eabi-objcopy -O binary $(ELF_TARGET) $(TARGET)

# Link the object files to create the ELF file
$(ELF_TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $(ELF_TARGET) $(OBJECTS)

# Compile assembly files into object files
$(OBJ_DIR)/%.o: $(SOURCES) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean target to remove generated files
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
