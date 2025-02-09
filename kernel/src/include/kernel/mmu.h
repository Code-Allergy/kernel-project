#ifndef KERNEL_MMU_H
#define KERNEL_MMU_H
#include "kernel/mm.h"
#include <stdint.h>
#include <stddef.h>
// #include <kernel/paging.h>

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

#define MMU_DOMAIN_KERNEL    0

// Domain Settings
#define MMU_DOMAIN_DEVICE    1           // Domain 1 for kernel
#define DACR_CLIENT_DOMAIN_KERNEL  (1 << MMU_DOMAIN_KERNEL)    // Client access for domain kernel
#define DACR_MANAGER_DOMAIN_KERNEL (3 << MMU_DOMAIN_KERNEL)    // Manager access for domain kernel

#define MMU_DOMAIN_USER      2           // Domain 2 for user
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

typedef uint32_t l2_page_table_t[256];
// // L1 Page Table (4096 entries, 4KB each for 4GB address space)
extern uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
extern l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

void test_domain_protection(void);
void debug_l1_l2_entries(void *va, uint32_t* ttbr0);

typedef struct {
    // Configuration state
    uint32_t kernel_page_table_paddr;  // Kernel page table address
    uint32_t* ttbr0;             // Translation Table Base Register 0 value
    uint32_t* ttbr1;             // Translation Table Base Register 1 value
    uint32_t domain_access;      // Domain access control register value
    uint32_t control_register;   // Control register value

    // Memory management info
    size_t total_pages;         // Total number of pages managed
    size_t free_pages;          // Number of available pages
    size_t kernel_pages;        // Pages used by kernel

    // Architecture specifics
    uint32_t page_size;         // Size of a memory page
    uint32_t section_size;      // Size of a section
    uint8_t max_domains;        // Maximum number of domains supported

    // driver status
    uint32_t initialized;       // Flag to indicate if the driver is initialized
    uint32_t enabled;           // Flag to indicate if the MMU is enabled

    // Basic MMU control functions
    void (*init)(void);
    void (*enable)(void);
    void (*disable)(void);
    void (*flush_tlb)(void);

    // will map a page to a physical address with l1_table table.
    void (*map_page)(void* l1_table, void* vaddr, void* paddr, uint32_t flags);

    // will unmap a page from l1_table, otherwise using kernel pages if is null.
    void (*unmap_page)(void* l1_table, void* vaddr);

    // get the physical address of a virtual address for the ttbr0 table.
    void* (*get_physical_address)(uint32_t* ttbr0, void* vaddr);

    void (*set_l1_table)(uint32_t* l1_table);
    void (*set_l1_with_asid)(uint32_t* table, uint8_t asid);
    void (*map_hardware_pages)(void);

    // // Domain access control
    // void (*set_domain_access)(uint32_t domain, uint32_t access);
    // uint32_t (*get_domain_access)(uint32_t domain);

    // // Cache operations
    // void (*invalidate_cache)(void);
    // void (*clean_cache)(void);
    // void (*flush_cache)(void);

    // // TLB operations
    // void (*invalidate_tlb_entry)(void* vaddr);
    // void (*set_ttbr0)(uint32_t ttbr0);
    // uint32_t (*get_ttbr0)(void);

    // // Memory attribute control
    // void (*set_memory_attributes)(void* vaddr, uint32_t size, uint32_t attrs);
    // uint32_t (*get_memory_attributes)(void* vaddr);

    // // Status and debug
    // uint32_t (*get_fault_status)(void);
    // void* (*get_fault_address)(void);
    // void (*debug_dump)(void)
} mmu_t;

extern mmu_t mmu_driver;

#endif // KERNEL_MMU_H
