#ifndef DRAM_H
#define DRAM_H

#include <stdint.h>

#define DRAMC_BASE 0x01c01000

typedef struct {
    volatile uint32_t SDR_CCR; //0x00
    volatile uint32_t SDR_DCR; //0x04
    volatile uint32_t SDR_IOCR; //0x08
    volatile uint32_t SDR_CSR; //0x0c
    volatile uint32_t SDR_DRR; //0x10
    volatile uint32_t SDR_TPR0; //0x14
    volatile uint32_t SDR_TPR1; //0x18
    volatile uint32_t SDR_TPR2; //0x1c
    uint32_t __reserved0[11]; //0x20 - 0x48
    volatile uint32_t SDR_RSLR0; //0x4c
    volatile uint32_t SDR_RSLR1; //0x50
    uint32_t __reserved1[2]; //0x54 - 0x58
    volatile uint32_t SDR_RDGR0; //0x5c
    volatile uint32_t SDR_RDGR1; //0x60
    uint32_t __reserved2[13]; //0x64 - 0x94
    volatile uint32_t SDR_ODTCR; //0x98
    volatile uint32_t SDR_DTR0; //0x9c
    volatile uint32_t SDR_DTR1; //0xa0
    volatile uint32_t SDR_DTAR; //0xa4
    volatile uint32_t SDR_ZQCR0; //0xa8
    volatile uint32_t SDR_ZQCR1; //0xac
    volatile uint32_t SDR_ZQSR; //0xb0
    volatile uint32_t SDR_IDCR; //0xb4
    uint32_t __reserved3[78]; //0xb8 - 0x1ec
    volatile uint32_t SDR_MR; //0x1f0
    volatile uint32_t SDR_EMR; //0x1f4
    volatile uint32_t SDR_EMR2; //0x1f8
    volatile uint32_t SDR_EMR3; //0x1fc

    volatile uint32_t SDR_DLLGCR; //0x200
    volatile uint32_t SDR_DLLCR0; //0x204
    volatile uint32_t SDR_DLLCR1; //0x208
    volatile uint32_t SDR_DLLCR2; //0x20c
    volatile uint32_t SDR_DLLCR3; //0x210
    volatile uint32_t SDR_DLLCR4; //0x214
    volatile uint32_t SDR_DQRT0; //0x218
    volatile uint32_t SDR_DQRT1; //0x21c
    volatile uint32_t SDR_DQRT2; //0x220
    volatile uint32_t SDR_DQRT3; //0x224
    volatile uint32_t SDR_DQSTR0; //0x228
    volatile uint32_t SDR_DQSTR1; //0x22c

    volatile uint32_t SDR_CR; //0x230
    volatile uint32_t SDR_CFSR; //0x234
    uint32_t         __reserved4; // 0x238
    volatile uint32_t SDR_DPCR; //0x23c
    volatile uint32_t SDR_APR; //0x240
    volatile uint32_t SDR_TLR; //0x244
    uint32_t         __reserved5[2]; // 0x24c
    volatile uint32_t SDR_HPCR; //0x250
    uint32_t        __reserved6[35]; // 0x254 - 0x2dc
    volatile uint32_t SDR_SCSR; //0x2e0
} DRAM_Type;

#define DRAM_CTRL ((DRAM_Type *)DRAMC_BASE)


#endif
