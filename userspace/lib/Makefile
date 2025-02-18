
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

CFLAGS = -ffreestanding -nostdlib -nostartfiles -std=gnu99 -Wall -Wextra -Wpedantic -Iinclude
CFLAGS += -mcpu=cortex-a8 -mfloat-abi=soft


SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

.PHONY: all clean
all: libosc.a


libosc.a: $(OBJECTS)
	$(AR) rcs $@ $^


# build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(BUILD_DIR) libosc.a
