#ifndef ELF_H
#define ELF_H


// ELF Header
typedef struct {
    unsigned char e_ident[16]; // ELF identification
    unsigned short e_type;     // Object file type
    unsigned short e_machine;  // Machine type
    unsigned int e_version;    // Object file version
    unsigned int e_entry;      // Entry point address
    unsigned int e_phoff;      // Program header offset
    unsigned int e_shoff;      // Section header offset
    unsigned int e_flags;      // Processor-specific flags
    unsigned short e_ehsize;   // ELF header size
    unsigned short e_phentsize; // Size of program header entry
    unsigned short e_phnum;     // Number of program header entries
    unsigned short e_shentsize; // Size of section header entry
    unsigned short e_shnum;     // Number of section header entries
    unsigned short e_shstrndx;  // Section name string table index
} elf_header_t;

#endif // ELF_H