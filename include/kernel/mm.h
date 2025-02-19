#ifndef KERNEL_MM_H
#define KERNEL_MM_H

// #ifdef PLATFORM_QEMU

// #define MEMORY_USER_CODE_BASE 0x00010000
// #define MEMORY_USER_DATA_BASE 0x00100000
// #define MEMORY_USER_HEAP_BASE 0x01000000
// #define MEMORY_USER_STACK_BASE 0x7F000000


// TODO

#define MEMORY_USER_CODE_BASE    0x00010000   // Keep user code start
#define MEMORY_USER_DATA_BASE    0x00100000   // Keep user data start
#define MEMORY_USER_HEAP_BASE    0x01000000   // Keep user heap start
#define MEMORY_USER_STACK_BASE   0x30000000   // Move stack below kernel
#define KERNEL_DIVIDER           0x80000000   // Keep kernel and user space separate
#define KERNEL_VIRTUAL_BASE      0x80000000   // Kernel virtual address space
#define KERNEL_VIRTUAL_DRAM      0x80000000   // Kernel virtual address space
#define KERNEL_START             0xC0000000   // TODO this should be virtual memory mapped instead of identity mapped
#define MEMORY_KERNEL_DEVICE_BASE 0xF0000000  // map devices as offset from this address


#define KERNEL_DRAM_OFFSET ((KERNEL_VIRTUAL_DRAM - KERNEL_VIRTUAL_BASE) + DRAM_BASE)
#define PHYS_TO_KERNEL_VIRT(x) ((uint32_t*)(((uint32_t)x) + KERNEL_DRAM_OFFSET))
#define KERNEL_VIRT_TO_PHYS(x) ((x) - KERNEL_DRAM_OFFSET)
#endif // KERNEL_MMU_H
