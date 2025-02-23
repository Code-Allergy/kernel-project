#include <stdint.h>
#include <stddef.h>
#include <utils.h>
#include <kernel/string.h>
#include <kernel/panic.h>

#include "dma.h"


const dma_desc_t dma_descriptors[32] __attribute__((aligned(32))); // Cache-aligned

// TODO - load kernel with dma instead of by sector, can enable interrupts in bootloader to make things easier
void dma_memory_write(uintptr_t desc_addr,
                      const dma_desc_t *desc, size_t len)
{
    (void)len;
    panic("DMA not implemented yet\n");

    /* 1. Convert descriptor to little-endian format */
    uint32_t le_desc[4] = {
        cpu_to_le32(desc->status),
        cpu_to_le32(desc->size),
        cpu_to_le32(desc->addr),
        cpu_to_le32(desc->next)
    };

    /* 3. Write descriptor to memory */
    memcpy((void*)desc_addr, desc, sizeof(le_desc));

    /* 4. Flush cache if needed (ARM-specific) */
    __asm__ volatile("dsb st" ::: "memory");
}
