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


#define MMU_DOMAIN_MANAGER (0x3)
#define MMU_DOMAIN_CLIENT (0x1)
#define MMU_DOMAIN_NO_ACCESS (0x0)

#define MMU_DOMAIN_DEVICE    2

// Domain Settings
#define MMU_DOMAIN_KERNEL    1           // Domain 1 for kernel
#define DACR_CLIENT_DOMAIN_KERNEL  (1 << MMU_DOMAIN_KERNEL)    // Client access for domain kernel
#define DACR_MANAGER_DOMAIN_KERNEL (3 << MMU_DOMAIN_KERNEL)    // Manager access for domain kernel

#define MMU_DOMAIN_USER      0           // Domain 2 for user
#define DACR_CLIENT_DOMAIN_USER    (1 << MMU_DOMAIN_USER)      // Client access for domain user
#define DACR_MANAGER_DOMAIN_USER   (3 << MMU_DOMAIN_USER)      // Manager access for domain user

// Access Permissions
#define MMU_AP_RW           (3 << 8)    // Read/write access
#define MMU_AP_RO           (2 << 8)    // Read-only access
#define MMU_AP_NO_ACCESS    (0 << 8)    // No access

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
#define L2_DEVICE_PAGE (L2_SMALL_PAGE | MMU_AP_RW | MMU_DEVICE_MEMORY | MMU_SHAREABLE)
#define L2_KERNEL_CODE_PAGE (L2_SMALL_PAGE | MMU_AP_RO | MMU_CACHEABLE | MMU_BUFFERABLE)
#define L2_KERNEL_DATA_PAGE (L2_SMALL_PAGE | MMU_AP_RW | MMU_CACHEABLE | MMU_BUFFERABLE)

// TEX[2:0] Memory Type Extensions
#define MMU_TEX(x)          ((x) << 12)  // TEX bits
#define MMU_TEX_DEVICE      (0 << 12)    // Strongly-ordered
#define MMU_TEX_NORMAL      (1 << 12)    // Normal memory

// Access Control Extensions
#define MMU_nG              (1 << 17)    // Non-global bit
#define MMU_S               (1 << 16)     // Shareable
#define MMU_AP2            (1 << 15)     // Access Permission extension

#define MMU_TABLE_ALIGN  __attribute__((aligned(16384)))

#define SECTION_INDEX(addr) ((addr) >> 20)
#define PAGE_INDEX(addr) (((addr) & 0xFFFFF) >> 12)



void map_page(uint32_t *ttbr0, void* vaddr, void* paddr, uint32_t flags);
uint32_t alloc_l1_table(struct page_allocator *alloc);
void kernel_mmu_init(bootloader_t* bootloader_info);

typedef uint32_t l2_page_table_t[256];
// // L1 Page Table (4096 entries, 4KB each for 4GB address space)
extern uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
extern l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));
void test_domain_protection(void);

#endif // KERNEL_MMU_H
