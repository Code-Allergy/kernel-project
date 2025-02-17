#ifndef KERNEL_DRAM_H
#define KERNEL_DRAM_H

typedef struct dram_info {
    unsigned long start;
    unsigned long size;

    void (*init)(void);
    // void (*cleanup)(void);
    // void (*suspend)(void);
    // void (*resume)(void);
    // int (*get_size)(unsigned long *size);
    // int (*get_info)(void *info, size_t size);
} dram_driver_t;

extern dram_driver_t dram_driver;

#endif // KERNEL_DRAM_H
