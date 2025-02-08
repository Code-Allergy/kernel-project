#ifndef _DMA_H
#define _DMA_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t status;  // Control flags (e.g., last descriptor, interrupt)
    uint32_t size; // size of buffer
    uint32_t addr; // physical address of next descriptor
    uint32_t next; // next in chain
} dma_desc_t;

extern const dma_desc_t dma_descriptors[32] __attribute__((aligned(32))); // Cache-aligned
void dma_memory_write(uintptr_t desc_addr,
                      const dma_desc_t *desc, size_t len);

#endif // _DMA_H
