# Makefile for compiling tester.S with arm-none-eabi-gcc

# Compiler (you can change this if your toolchain has a different prefix)
CC = arm-none-eabi-gcc

# Compiler flags; adjust as needed (for assembly you might not need many flags)
CFLAGS = -O2 -Wall -mcpu=cortex-a8 -marm -nostdlib -ffreestanding

# Target binary name
TARGET = tester.elf

# Source file
SOURCES = tester.S

# Default target: build the binary
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

# Clean target to remove generated files
clean:
	rm -f $(TARGET)

.PHONY: all clean
