#ifndef ALLWINNER_A10_MMU_H
#define ALLWINNER_A10_MMU_H


// static inline void enable_mmu(void) {
// updatte this when we add L2 page table
#define PAGE_SIZE 0x100000

// }

// Basic Descriptors
#define MMU_SECTION_DESCRIPTOR (2 << 0)  // Section descriptor (b10)
#define MMU_PAGE_DESCRIPTOR   (1 << 0)   // Page descriptor (b01)
#define MMU_INVALID          (0 << 0)    // Invalid descriptor (b00)

// Domain Settings
#define MMU_DOMAIN_KERNEL    0           // Domain 0 for kernel
#define DACR_CLIENT_DOMAIN0  (1 << 0)    // Client access for domain 0
#define DACR_MANAGER_DOMAIN0 (3 << 0)    // Manager access for domain 0

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
#define MMU_SHAREABLE       (1 << 16)    // Mark as shareable
#define MMU_NON_SHAREABLE   (0 << 16)    // Mark as non-shareable
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



void mmu_init_page_table(bootloader_t* bootloader_info);
void mmu_enable();
void mmu_set_domains();

#endif // ALLWINNER_A10_MMU_H