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

const char* file_path = "/elf/null.elf";
static uint8_t bytes[16384];

int test_elf32(void) {
    fat32_fs_t sd_card;
    fat32_file_t userspace_application;
    if (fat32_mount(&sd_card, &mmc_fat32_diskio) != 0) {
        printk("Failed to mount SD card\n");
        return -1;
    }
    if (fat32_open(&sd_card, file_path, &userspace_application)) {
        printk("Failed to open file\n");
        return -1;
    }


    if ((uint32_t)fat32_read(&userspace_application, bytes, 16384) != userspace_application.file_size) {
        printk("Init process not read fully off disk\n");
        return -1;
    };

    printk("First bytes: %p\n", bytes[0]);
    elf_header_t* header = (elf_header_t*)bytes;
    elf_section_header_t* section_headers = (elf_section_header_t*)(bytes + header->e_shoff);
    elf_program_header_t* program_headers = (elf_program_header_t*)(bytes + header->e_phoff);

    printk("e_ident: %s\n", header->e_ident);
    // Print the ELF header information
    printk("ELF Header:\n");
    // printk("  Magic:   %.4s\n", (char*)header->e_ident.magic);
    printk("  File Type: %u\n", header->e_type);
    printk("  Machine: %u\n", header->e_machine);
    printk("  Entry point address: 0x%x\n", header->e_entry);
    printk("  Program header offset: 0x%x\n", header->e_phoff);
    printk("  Section header offset: 0x%x\n", header->e_shoff);
    printk("  Flags: 0x%x\n", header->e_flags);
    printk("  ELF header size: %u\n", header->e_ehsize);
    printk("  Program header entry size: %u\n", header->e_phentsize);
    printk("  Number of program headers: %u\n", header->e_phnum);
    printk("  Section header entry size: %u\n", header->e_shentsize);
    printk("  Number of section headers: %u\n", header->e_shnum);
    printk("  Section name string table index: %u\n", header->e_shstrndx);

    printk("Program Headers:\n");
    for (int i = 0; i < header->e_phnum; i++) {
        elf_program_header_t* phdr = &program_headers[i];

        printk("Program Header %d:\n", i);
        printk("  Type:      0x%x\n", phdr->p_type);
        printk("  Offset:    0x%x\n", phdr->p_offset);
        printk("  Virtual Addr: 0x%x\n", phdr->p_vaddr);
        printk("  Physical Addr: 0x%x\n", phdr->p_paddr);
        printk("  File Size: 0x%x\n", phdr->p_filesz);
        printk("  Mem Size:  0x%x\n", phdr->p_memsz);
        printk("  Flags:     0x%x\n", phdr->p_flags);
        printk("  Align:     0x%x\n", phdr->p_align);
        printk("\n");
    }

    for (int i = 0; i < header->e_shnum; i++) {
        elf_section_header_t* shdr = &section_headers[i];

        printk("Section %d:\n", i);
        // printk("  Name:   %s\n", section_name); // TODO
        printk("  Type:   %u\n", shdr->sh_type);
        printk("  Flags:  0x%x\n", shdr->sh_flags);
        printk("  Address: 0x%x\n", shdr->sh_addr);
        printk("  Offset: 0x%x\n", shdr->sh_offset);
        printk("  Size:   0x%x\n", shdr->sh_size);
        printk("  Link:   %u\n", shdr->sh_link);
        printk("  Info:   %u\n", shdr->sh_info);
        printk("  Align:  %u\n", shdr->sh_addralign);
        printk("  Entry Size: %u\n", shdr->sh_entsize);
        printk("\n");
    }

    elf_section_header_t* string_table_header = &section_headers[header->e_shstrndx];
    char* string_table = (char*)(bytes + string_table_header->sh_offset);
    printk("Section Names:\n");
    for (int i = 0; i < header->e_shnum; i++) {
        elf_section_header_t* section = &section_headers[i];
        const char* section_name = &string_table[section->sh_name];
        printk("Section %d: %s\n", i, section_name);
    }

    return 0;
}

// TODO we always assume that the file is parsed correctly
binary_t* load_elf32(uint8_t* bytes, size_t size) {
    // check if the file is an ELF file
    elf_header_t* header = (elf_header_t*)bytes;
    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        return NULL;
    }

    // check that the file is a 32bit file
    if (header->e_ident[4] != 1) {
        return NULL;
    }

    // check if the architecture is arm
    if (header->e_machine != EM_ARM) {
        panic("Unsupported architecture\n");
        return NULL;
    }


    binary_t* binary = (binary_t*)kmalloc(sizeof(binary_t));
    if (binary == NULL) {
        return NULL;
    }

    binary->type = BINARY_TYPE_ELF32;
    binary->entry = header->e_entry;

    binary->data.elf.raw = bytes;
    binary->data.elf.size = size;
    binary->data.elf.file_type = header->e_type;
    binary->data.elf.architecture = header->e_machine;

    binary->data.elf.header = (elf_header_t*) bytes;
    binary->data.elf.program_headers = (elf_program_header_t*)(bytes + header->e_phoff);
    binary->data.elf.program_header_count = header->e_phnum;
    binary->data.elf.section_headers = (elf_section_header_t*)(bytes + header->e_shoff);
    binary->data.elf.section_header_count = header->e_shnum;
    binary->data.elf.string_table = (uint8_t*) (bytes + binary->data.elf.section_headers[header->e_shstrndx].sh_offset);

    // FLAGS


    // CHECKSUM


    return binary;
}
