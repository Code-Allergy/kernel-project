#include <kernel/printk.h>
#include <kernel/dram.h>
#include "dram.h"
#include "ccm.h"

void dram_init(void) {
    // QEMU dramc emulation for A10 is really poor and does nothing
    // https://github.com/qemu/qemu/blob/master/hw/misc/allwinner-a10-dramc.c

    // check if the qemu version is even emulating dramc
    if (DRAM_CTRL->SDR_CCR != 0x80020000) {
        printk("Skipping initializing dram controller, qemu verison unsupported.\n");
        return -1;
    }

    DRAM_CTRL->SDR_CCR = (1 << 31);
    while ((DRAM_CTRL->SDR_CCR & (1 << 31)));
    DRAM_CTRL->SDR_ZQCR0 = 0x1;
    while ((DRAM_CTRL->SDR_ZQCR0 & (1 << 31)));

    return 0;
}


dram_driver_t dram_driver = {
    .start = 0x40000000,
    .size = 0x20000000,
    .init = dram_init,
    // void (*cleanup)(void);
    // void (*suspend)(void);
    // void (*resume)(void);
    // int (*get_size)(unsigned long *size);
    // int (*get_info)(void *info, size_t size);
};
