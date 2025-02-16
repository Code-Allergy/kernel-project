#include <kernel/printk.h>
#include <kernel/dram.h>

void dram_init(void) {
    printk("Skipping dram init on BBB\n");
}


dram_driver_t dram_driver = {
    .start = 0x80000000,
    .size = 0x20000000,
    .init = dram_init,
    // void (*cleanup)(void);
    // void (*suspend)(void);
    // void (*resume)(void);
    // int (*get_size)(unsigned long *size);
    // int (*get_info)(void *info, size_t size);
};
