#include <kernel/uart.h>
#include <kernel/dram.h>
#include <kernel/ccm.h>
#include <kernel/printk.h>
#include <kernel/mmc.h>
#include <kernel/fat32.h>
#include <kernel/boot.h>
#include <kernel/mmu.h>
#include <stdint.h>

#include "drivers/bbb/eeprom.h"

extern uintptr_t init;
BOARDINFO board_info;
#define I2C_SLAVE_ADDR                      (0x50)

/* bootloader C entry point */
void loader(void){
    fat32_fs_t boot_fs;
    fat32_file_t kernel;
    int res = 0;

    uart_driver.init();
    printk("UART Active\n");
    printk("Loader loaded at %p\n", (void*)init);
    printk("Build time (UTC): %s\n", BUILD_DATE);

    eeprom_init(I2C_SLAVE_ADDR);
    eeprom_read((void*)&board_info, MAX_DATA, 0);
    printk("EEPROM header: %.4s\n", board_info.header);
    printk("Board name: %.8s\n", board_info.boardName);
    printk("Version: %.5s\n", board_info.version);
    printk("Serial Number %.12s\n", board_info.serialNumber);

    ccm_driver.init();

    dram_driver.init();
    printk("DRAM Active\n");

    // mmc_driver.init();
    printk("Done on BBB\n");

#ifndef PLATFORM_BBB
    CHECK_FAIL(fat32_mount(&boot_fs, &mmc_fat32_diskio), "Failed to mount FAT32 filesystem");
    CHECK_FAIL(fat32_open(&boot_fs, KERNEL_PATH, &kernel), "Failed to open kernel image");
    CHECK_FAIL(kernel.file_size == 0, "Kernel image is empty");



    // enable mmu, enable ram
    mmu_driver.init();
    // identity map all of dram, so we can access the kernel


    // map the rest of the memory into kernel space for the jump to kernel.
    for (uintptr_t i = DRAM_BASE; i < DRAM_BASE + DRAM_SIZE; i += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)(KERNEL_ENTRY + (i - DRAM_BASE)), (void*)i, L2_KERNEL_DATA_PAGE);
    }

    // identity map DRAM, so we can access the bootloader (unneeded on BBB, we are on other memory)
    for (uintptr_t i = 0; i < DRAM_SIZE; i += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)(i + DRAM_BASE), (void*)(i + DRAM_BASE), L2_KERNEL_DATA_PAGE);
    }

    mmu_driver.enable();

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
