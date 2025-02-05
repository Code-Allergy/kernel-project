#ifndef KERNEL_MMU_H
#define KERNEL_MMU_H
#include <stdint.h>
#include <kernel/paging.h>

#define PAGE_SIZE 4096

// Basic Descriptors
#define MMU_SECTION_DESCRIPTOR (2 << 0)  // Section descriptor (b10)
#define MMU_PAGE_DESCRIPTOR   (1 << 0)   // Page descriptor (b01)
#define MMU_INVALID          (0 << 0)    // Invalid descriptor (b00)

// L2 Page Table Descriptors
#define L2_SMALL_PAGE        (2 << 0)    // Small page descriptor (b10)
#define L2_LARGE_PAGE        (1 << 0)    // Large page descriptor (b01)


// Domain Settings
#define MMU_DOMAIN_KERNEL    1           // Domain 1 for kernel
#define DACR_CLIENT_DOMAIN1  (1 << 1)    // Client access for domain 1
#define DACR_MANAGER_DOMAIN1 (3 << 1)    // Manager access for domain 1

// Access Permissions
#define MMU_AP_RW           (3 << 10)    // Read/write access
#define MMU_AP_RO           (2 << 10)    // Read-only access
#define MMU_AP_NO_ACCESS    (0 << 10)    // No access

// Memory Type
#define MMU_CACHEABLE       (1 << 3)     // Enable caching
#define MMU_BUFFERABLE      (1 << 2)     // Enable write buffering
#define MMU_DEVICE_MEMORY   (0 << 3)     // Disable caching for device memory
#define MMU_NON_CACHEABLE   (0 << 3)     // Disable caching

// Shareability and Execute Permissions
#define MMU_SHAREABLE       (1 << 10)    // Mark as shareable
#define MMU_NON_SHAREABLE   (1 << 10)    // Mark as non-shareable
#define MMU_EXECUTE_NEVER   (1 << 4)     // Prevent instruction fetches

// Memory Region Types
#define MMU_NORMAL_MEMORY   (MMU_CACHEABLE | MMU_BUFFERABLE)
#define MMU_DEVICE_SHARED   (MMU_DEVICE_MEMORY | MMU_SHAREABLE)
#define MMU_DEVICE_NON_SHARED (MMU_DEVICE_MEMORY)

// TEX[2:0] Memory Type Extensions
#define MMU_TEX(x)          ((x) << 12)  // TEX bits
#define MMU_TEX_DEVICE      (0 << 12)    // Strongly-ordered
#define MMU_TEX_NORMAL      (1 << 12)    // Normal memory

// Access Control Extensions
#define MMU_nG              (1 << 17)    // Non-global bit
#define MMU_S               (1 << 16)     // Shareable
#define MMU_AP2            (1 << 15)     // Access Permission extension

#define MMU_TABLE_ALIGN  __attribute__((aligned(16384)))

void map_page(uint32_t *ttbr0, void* vaddr, void* paddr, uint32_t flags);
uint32_t alloc_l1_table(struct page_allocator *alloc);
void kernel_mmu_init(bootloader_t* bootloader_info);

typedef uint32_t l2_page_table_t[256];
// // L1 Page Table (4096 entries, 4KB each for 4GB address space)
// extern uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
// extern l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

#endif // KERNEL_MMU_H
