#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>


#define ELF_PROGRAM_HEADER_FLAG_EXECUTABLE 0x1
#define ELF_PROGRAM_HEADER_FLAG_WRITABLE 0x2
#define ELF_PROGRAM_HEADER_FLAG_READABLE 0x4

#define ELF_PROGRAM_HEADER_TYPE_NULL 0
#define ELF_PROGRAM_HEADER_TYPE_LOAD 1
#define ELF_PROGRAM_HEADER_TYPE_DYNAMIC 2
#define ELF_PROGRAM_HEADER_TYPE_INTERP 3
#define ELF_PROGRAM_HEADER_TYPE_NOTE 4
#define ELF_PROGRAM_HEADER_TYPE_SHLIB 5

// #define ELF_SECTION_HEADER_FLAG_WRITABLE 0x1
// #define ELF_SECTION_HEADER_FLAG_ALLOCATED 0x2
// #define ELF_SECTION_HEADER_FLAG_EXECUTABLE 0x4
// #define ELF_SECTION_HEADER_FLAG_MERGE 0x10
// #define ELF_SECTION_HEADER_FLAG_STRINGS 0x20
// #define ELF_SECTION_HEADER_FLAG_INFO_LINK 0x40
// #define ELF_SECTION_HEADER_FLAG_LINK_ORDER 0x80

struct _eident {
    unsigned char ei_mag[4]; // File identification
    unsigned char ei_class;  // File class
    unsigned char ei_data;   // Data encoding
    unsigned char ei_version; // File version
    unsigned char ei_osabi;   // OS/ABI identification
    unsigned char __padding[8]; // Padding
};

typedef struct {
    uint32_t p_type;   // Type of segment (e.g., loadable segment)
    uint32_t p_offset; // Offset in the file
    uint32_t p_vaddr;  // Virtual address in memory
    uint32_t p_paddr;  // Physical address (not typically used in modern systems)
    uint32_t p_filesz; // Size of the segment in the file
    uint32_t p_memsz;  // Size of the segment in memory
    uint32_t p_flags;  // Segment flags (e.g., read/write/execute)
    uint32_t p_align;  // Alignment of the segment
} elf_program_header_t;


typedef struct {
    uint32_t sh_name;      // Section name (index into the string table)
    uint32_t sh_type;      // Section type
    uint32_t sh_flags;     // Section flags
    uint32_t sh_addr;      // Address in memory
    uint32_t sh_offset;    // Offset in the file
    uint32_t sh_size;      // Size of section
    uint32_t sh_link;      // Link to another section (e.g., for relocation)
    uint32_t sh_info;      // Extra information (e.g., for relocation)
    uint32_t sh_addralign; // Section alignment
    uint32_t sh_entsize;   // Size of entries (if the section holds a table)
} elf_section_header_t;

// ELF Header
typedef struct {
    uint8_t e_ident[16]; // ELF identification
    uint16_t e_type;     // Object file type
    uint16_t e_machine;  // Machine type
    uint32_t e_version;    // Object file version
    uint32_t e_entry;      // Entry point address
    uint32_t e_phoff;      // Program header offset
    uint32_t e_shoff;      // Section header offset
    uint32_t e_flags;      // Processor-specific flags
    uint16_t e_ehsize;   // ELF header size
    uint16_t e_phentsize; // Size of program header entry
    uint16_t e_phnum;     // Number of program header entries
    uint16_t e_shentsize; // Size of section header entry
    uint16_t e_shnum;     // Number of section header entries
    uint16_t e_shstrndx;  // Section name string table index
} elf_header_t;

typedef enum {
    BINARY_TYPE_FLAT,
    BINARY_TYPE_ELF32,
} binary_type_t;

typedef struct {
    size_t size;
    uint8_t* raw;
    uint16_t file_type;
    uint16_t architecture;

    // references into the raw data
    elf_header_t* header; // ELF header (points to raw)
    elf_program_header_t* program_headers;
    size_t program_header_count;
    elf_section_header_t* section_headers;
    size_t section_header_count;
    uint8_t* string_table;
    uint32_t flags;
    uint8_t checksum[16];
} elf_binary_t;

typedef struct {
    size_t size;
    uint8_t* raw;
} flat_binary_t;

typedef union {
    elf_binary_t elf;  // ELF binary data
    flat_binary_t flat; // Flat binary data
} binary_data_t;

typedef struct {
    binary_type_t type;   // Type to identify the kind of binary
    uint32_t entry;       // Entry point
    binary_data_t data;   // Union of different binary types
} binary_t;




int test_elf32(void);
binary_t* load_elf32(uint8_t* bytes, size_t size);

#endif // ELF_H
