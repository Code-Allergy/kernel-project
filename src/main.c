#include <kernel/boot.h>


int kernel_init() {
    return 0;
}

__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    if (bootloader_info->magic != 0xFEEDFACE) {
        printk("Invalid bootloader magic: %x\n", bootloader_info->magic);
        return -1;
    }

    printk("Kernel init size: %d\n", bootloader_info->kernel_size);
    kernel_init();


    // scheduler_init();
    // if we return from this function, we should halt the system.

    printk("Kernel exit\n");
    asm volatile("wfi");
    __builtin_unreachable();
}
