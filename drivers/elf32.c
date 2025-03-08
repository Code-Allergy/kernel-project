#include <stdint.h>
#include <stddef.h>
#include <elf32.h>

#include <kernel/fat32.h>
#include <kernel/printk.h>
#include <kernel/errno.h>
#include <kernel/paging.h>
#include <kernel/mmc.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/panic.h>

#define EM_ARM 0x28
#define EM_AARCH64 0xB7

binary_t* load_elf32(uint8_t* bytes, size_t size) {
    // Basic size check
    if (size < sizeof(elf_header_t)) {
        printk("ELF file too small\n");
        return NULL;
    }

    // Check if the file is an ELF file
    elf_header_t* header = (elf_header_t*)bytes;
    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        printk("Not an ELF file\n");
        return NULL;
    }

    // Check that the file is a 32bit file
    if (header->e_ident[4] != 1) {
        printk("Not a 32-bit ELF file\n");
        return NULL;
    }

    // Check if the architecture is ARM
    if (header->e_machine != EM_ARM) {
        printk("Unsupported architecture: %d\n", header->e_machine);
        return NULL;
    }

    // Validate program header information
    if (header->e_phentsize != sizeof(elf_program_header_t)) {
        printk("Invalid program header size\n");
        return NULL;
    }

    // Check program headers are within file bounds
    if (header->e_phoff == 0 ||
        header->e_phnum == 0 ||
        header->e_phoff + header->e_phnum * header->e_phentsize > size) {
        printk("Program headers out of bounds\n");
        return NULL;
    }

    // Check section headers are within file bounds
    if (header->e_shoff != 0 &&
        header->e_shnum != 0 &&
        header->e_shoff + header->e_shnum * header->e_shentsize > size) {
        printk("Section headers out of bounds\n");
        return NULL;
    }

    // Validate string table section index
    if (header->e_shstrndx >= header->e_shnum) {
        printk("Invalid string table index\n");
        return NULL;
    }

    // Allocate binary structure
    binary_t* binary = (binary_t*)kmalloc(sizeof(binary_t));
    if (binary == NULL) {
        printk("Failed to allocate memory for binary\n");
        return NULL;
    }

    // Initialize basic information
    binary->type = BINARY_TYPE_ELF32;
    binary->entry = header->e_entry;
    binary->data.elf.raw = bytes;
    binary->data.elf.size = size;
    binary->data.elf.file_type = header->e_type;
    binary->data.elf.architecture = header->e_machine;
    binary->data.elf.header = header;

    // Set up program headers
    binary->data.elf.program_headers = (elf_program_header_t*)(bytes + header->e_phoff);
    binary->data.elf.program_header_count = header->e_phnum;

    // Set up section headers if present
    if (header->e_shnum > 0) {
        binary->data.elf.section_headers = (elf_section_header_t*)(bytes + header->e_shoff);
        binary->data.elf.section_header_count = header->e_shnum;

        // Validate string table section
        elf_section_header_t* strtab = &binary->data.elf.section_headers[header->e_shstrndx];
        if (strtab->sh_offset + strtab->sh_size > size) {
            printk("String table out of bounds\n");
            kfree(binary);
            return NULL;
        }
        binary->data.elf.string_table = bytes + strtab->sh_offset;
    } else {
        binary->data.elf.section_headers = NULL;
        binary->data.elf.section_header_count = 0;
        binary->data.elf.string_table = NULL;
    }

    return binary;
}
