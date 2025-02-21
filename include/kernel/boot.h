#ifndef KERNEL_BOOT_H
#define KERNEL_BOOT_H

#include <stdint.h>
#include <utils.h>
#include <kernel/mm.h>

extern void setup_stacks(void);

#ifndef KERNEL_ENTRY
  #ifdef PLATFORM_QEMU
    #define KERNEL_ENTRY 0xC0000000
  #elif defined(PLATFORM_BBB)
    #define KERNEL_ENTRY 0x80000000
  #endif
#endif

#ifndef DRAM_BASE
  #ifdef PLATFORM_QEMU
    #define DRAM_BASE 0x40000000
  #elif defined(PLATFORM_BBB)
    #define DRAM_BASE 0x80000000
  #else
    #define DRAM_BASE 0x40000000
  #endif
#endif

#ifndef DRAM_SIZE
  #define DRAM_SIZE 0x20000000
#endif

#ifdef BOOTLOADER
  #define IO_KERNEL_OFFSET 0
#else
  #define IO_KERNEL_OFFSET 0xF0000000
#endif

#define KERNEL_PATH "/boot/kernel.bin"

// temporary stack size
#define STACK_CANARY_VALUE 0xDEADBEEF
#define KERNEL_STACK_SIZE (32 * 1024)  // 32KB -- we can set this on the large size for now

typedef int (*entry_t)(void);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define BUILD_DATE __DATE__ " " __TIME__

#define BOOTFAILED(str) do { \
    printk("Bootloader failed at stage: " str " on line " TOSTRING(__LINE__) " at file " __FILE__ "\n"); \
    goto bootloader_fail; \
} while (0)

typedef struct {
    uint32_t magic;
    char* build_time;
    char* kernel_version;  // TODO implement different versioning
    uint32_t kernel_size;
    uint32_t kernel_entry;
    uint32_t kernel_checksum;
    uint32_t kernel_flags;     // TODO implement flags
    uint32_t kernel_end;
    uint32_t total_memory;
    uint32_t reserved_memory; // kernel size, bootloader size already omitted for qemu
    uint32_t l1_table_base;
    uint32_t l1_table_size;
    uint32_t l2_table_base;
    uint32_t l2_table_size;
} bootloader_t;


#define CHECK_FAIL(call, msg) if ((res = (call)) < 0) { \
    printk("Bootloader failed at stage: " msg " (Error %d) on line %d in file %s\n", res, __LINE__, __FILE__); \
    goto bootloader_fail; \
}

// TODO checksum, versioning, flags, other things
#define JUMP_KERNEL(kernel) do {  \
    bootloader_t bootloader = {   \
        .magic = 0xFEEDFACE,      \
        .build_time = "Build: " BUILD_DATE,    \
        .kernel_version = '\0',                \
        \
        .kernel_size = (kernel).file_size,\
        .kernel_entry = KERNEL_ENTRY, \
        .kernel_checksum = calculate_checksum((void*)KERNEL_ENTRY, (kernel).file_size),       \
        .kernel_flags = 0,                  \
        .kernel_end = DRAM_BASE + (kernel).file_size, \
        .total_memory = DRAM_SIZE,          \
        .reserved_memory = (kernel).file_size, \
        .l1_table_base = l1_page_table, \
        .l1_table_size = 0x4000, \
        .l2_table_base = l2_tables, \
        .l2_table_size = 0x400000, \
    };\
    fat32_close(&kernel);\
    printk("Kernel size: %d\n", bootloader.kernel_size); \
    printk("Kernel entry: %p\n", bootloader.kernel_entry); \
    printk("Kernel end: %p\n", bootloader.kernel_end); \
    printk("Kernel flags %d\n", bootloader.kernel_flags); \
    printk("Total memory: %dM (%d)\n", bootloader.total_memory / (1024*1024), bootloader.total_memory); \
    printk("Reserved memory: %d\n", bootloader.reserved_memory); \
    printk("Entering kernel \nFirst instructions: %p %p %p %p\n",\
        *(uint32_t*)bootloader.kernel_entry,          \
       (*(uint32_t*)(bootloader.kernel_entry + 4)),   \
       (*(uint32_t*)(bootloader.kernel_entry + 8)),   \
       (*(uint32_t*)(bootloader.kernel_entry + 12))); \
    __asm__ volatile (          \
        "mov r0, %0\n\t"        \
        "bx %1"                 \
        :                       \
        : "r" (&bootloader), \
          "r" (KERNEL_ENTRY)   \
        : "r0", "memory"  \
    );\
    res = ((entry_t)KERNEL_ENTRY)(); \
} while (0)

extern bootloader_t bootloader_info;

#endif
