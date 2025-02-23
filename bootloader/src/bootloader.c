#include <stdint.h>

#include <kernel/uart.h>
#include <kernel/dram.h>
#include <kernel/ccm.h>
#include <kernel/printk.h>
#include <kernel/mmc.h>
#include <kernel/fat32.h>
#include <kernel/boot.h>
#include <kernel/mmu.h>
#include <kernel/board.h>
#include <kernel/i2c.h>
#include <kernel/string.h>

#ifdef PLATFORM_BBB
unsigned int UARTBootCopy(void);
#define SECTION_ENTRY_NORMAL (0x2 | (0x3 << 10) | (0x1 << 12) | (1 << 3) | (1 << 2))
#define SECTION_ENTRY_DEVICE (0x2 | (0x3 << 10) | (0x0 << 12))
#endif

// no frame allocator setup in the bootloader
uint32_t kernel_end = DRAM_BASE + DRAM_SIZE;

// these are unused by the bootloader, but are needed for the kernel
uint32_t kernel_end;
uint32_t kernel_code_end;



/* bootloader C entry point */
void loader(void){

    // fat32 driver
    fat32_fs_t boot_fs;
    fat32_file_t kernel;
    int res = 0;
    uart_driver.init();
    board_info.init();

    printk("UART Active\n");
    printk("Loader loaded at %p\n", (void*)(uintptr_t)loader);
    printk("Build time (UTC): %s\n", BUILD_DATE);

    printk("Board name: %s\n", board_info.name);
    printk("Version: %s\n", board_info.version);
    printk("Serial Number %s\n", board_info.serial_number);
    ccm_driver.init();
    dram_driver.init();
    mmc_driver.init();
    printk("Done on BBB - need working MMU/MMC reads\n");

    mmu_driver.init();
#ifdef PLATFORM_BBB
    // clear l1 and l2 tables
    // memset((void*)l1_page_table, 0, 0x4000);
    // memset((void*)l2_tables, 0, 0x400000);

    // Copy the kernel from the SD card to the DRAM
    if (UARTBootCopy() != 0) {
        printk("Failed to copy kernel from UART\n");
        return;
    }
    // Map critical regions
    for (uint32_t section = 0x800; section < 0x900; section++) {
        l1_page_table[section] = (section << 20) | SECTION_ENTRY_NORMAL;
    }
    l1_page_table[0xC00] = 0x80000000 | SECTION_ENTRY_NORMAL;  // 0xC0000000
    l1_page_table[0x480] = 0x48000000 | SECTION_ENTRY_DEVICE;  // GPIO
    l1_page_table[0x44E] = 0x44E00000 | SECTION_ENTRY_DEVICE;  // UART
    mmu_driver.enable();
    uint32_t *test_virt = (uint32_t*)0xC0000000; // Mapped to 0x80000000
    uint32_t *test_phys = (uint32_t*)0x80000000;   // Identity-mapped
    printk("Virtual: 0x%x | Physical: 0x%x\n", *test_virt, *test_phys);

    // jump to 0xC0000000
    void (*kernel_entry)(void) = (void (*)(void))(0x80000000);
    kernel_entry();
#endif
#ifdef PLATFORM_QEMU
    // map the rest of the memory into kernel space for the jump to kernel.
    for (uintptr_t i = DRAM_BASE; i < DRAM_BASE + DRAM_SIZE; i += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)(KERNEL_ENTRY + (i - DRAM_BASE)), (void*)i, L2_KERNEL_DATA_PAGE);
    }

    // identity map DRAM, so we can access the bootloader (unneeded on BBB, we are on other memory)
    for (uintptr_t i = 0; i < DRAM_SIZE; i += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)(i + DRAM_BASE), (void*)(i + DRAM_BASE), L2_KERNEL_DATA_PAGE);
    }

    mmu_driver.enable();
    CHECK_FAIL(fat32_mount(&boot_fs, &mmc_fat32_diskio), "Failed to mount FAT32 filesystem");
    CHECK_FAIL(fat32_open(&boot_fs, KERNEL_PATH, &kernel), "Failed to open kernel image");
    CHECK_FAIL(kernel.file_size == 0, "Kernel image is empty");

    /* Read kernel into DRAM */
    if ((res = fat32_read(&kernel, (void*)KERNEL_ENTRY, kernel.file_size)) < 0
        || res != (int)kernel.file_size) {
        printk("Bootloader failed: Failed to read entire kernel into memory! (Read %d bytes, expected %d)\n", res, kernel.file_size);
        goto bootloader_fail;
    }

    /* jump to kernel memory space */
    JUMP_KERNEL(kernel);
#endif
    // If we get here, the kernel failed to load, or bailed out
    printk("Failed to load kernel with error code %d! Halting!\n", res);
    while(1);

bootloader_fail:
    printk("Failed to init bootloader! Halting!\n");
    while(1);
}
