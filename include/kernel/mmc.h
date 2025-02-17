#ifndef KERNEL_MMC_H
#define KERNEL_MMC_H

typedef struct {
    void (*init)(void);
} mmc_driver_t;

extern mmc_driver_t mmc_driver;

#endif
