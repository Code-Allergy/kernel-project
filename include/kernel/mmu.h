#ifndef KERNEL_MMU_H
#define KERNEL_MMU_H
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

#define MMU_DOMAIN_USER      1           // Domain 1 for user
#define DACR_CLIENT_DOMAIN_USER    (1 << MMU_DOMAIN_USER)      // Client access for domain user
#define DACR_MANAGER_DOMAIN_USER   (3 << MMU_DOMAIN_USER)      // Manager access for domain user


// TODO fix!! shouldn't he << 8, but << 4 and 2 sections.
// Access Permissions
#define MMU_AP_RW           (3 << 8)    // Read/write access
#define MMU_AP_RO           (2 << 8)    // Read-only access
#define MMU_AP_NO_ACCESS    (0 << 8)    // No access
// #define MMU_AP_KERNEL_RW    (1 << 8)    // Kernel read/write access
// #define MMU_AP_KERNEL_RO    (1 << 8 | 1 << )    // Kernel read-only access
// #define MMU_AP_KERNEL_RW    (1 << 10)   // Kernel read/write access

// Memory Type
#define MMU_CACHEABLE       (1 << 3)     // Enable caching
#define MMU_BUFFERABLE      (1 << 2)     // Enable write buffering
#define MMU_DEVICE_MEMORY   (0 << 3)     // Disable caching for device memory
#define MMU_NON_CACHEABLE   (0 << 3)     // Disable caching

// Shareability and Execute Permissions
#define MMU_SHAREABLE       (1 << 10)    // Mark as shareable
#define MMU_NON_SHAREABLE   (1 << 10)    // Mark as non-shareable
#define MMU_EXECUTE_NEVER   (1 << 4)     // Prevent instruction fetches
#define MMU_EXECUTE         (0 << 4)     // Enable instruction fetches

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

#define SECTION_INDEX(addr) ((addr) >> 20)
#define PAGE_INDEX(addr) (((addr) & 0xFFFFF) >> 12)

/* TTBCR Masks and Shifts */
#define TTBCR_N_MASK     0x7
#define TTBCR_PD0_MASK   (1 << 4)
#define TTBCR_PD1_MASK   (1 << 5)
#define TTBCR_EAE_MASK   (1 << 31)

/* For splitting at 0x80000000, we need N=1 (2GB/2GB split) */
#define TTBCR_SPLIT_2GB   1

/* Helper macros for boundary calculations */
#define TTBR0_BOUNDARY(n) (0xFFFFFFFF >> (n))
#define TTBR1_BOUNDARY(n) (~TTBR0_BOUNDARY(n))



// arm specific
static inline void set_ttbr0(uint32_t val) {
    asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r" (val) : "memory");
}

static inline void set_ttbr1(uint32_t val) {
    asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r" (val) : "memory");
}

static inline uint32_t get_ttbr0(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (val));
    return val;
}

static inline uint32_t get_ttbr1(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r" (val));
    return val;
}

/**
 * Translation Table Base Control Register
 */
static inline void set_ttbcr(uint32_t val) {
    asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r" (val) : "memory");
}

static inline uint32_t get_ttbcr(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r" (val));
    return val;
}

/**
 * Domain Access Control Register
 */
static inline void set_dacr(uint32_t val) {
    asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r" (val) : "memory");
}

static inline uint32_t get_dacr(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (val));
    return val;
}

/**
 * MMU Control Functions
 */
static inline void enable_mmu(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (val));
    val |= 1; // Set M bit
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (val) : "memory");
}

static inline void disable_mmu(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (val));
    val &= ~1; // Clear M bit
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (val) : "memory");
}

/**
 * TLB Operations
 */
static inline void invalidate_tlb(void) {
    asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0) : "memory");
}

static inline void invalidate_tlb_single(uint32_t mva) {
    asm volatile ("mcr p15, 0, %0, c8, c7, 1" : : "r" (mva) : "memory");
}

/**
 * Cache Operations
 */
static inline void invalidate_icache(void) {
    asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0) : "memory");
}

static inline void invalidate_dcache(void) {
    asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0) : "memory");
}

/**
 * Data Memory Barrier
 */
static inline void dmb(void) {
    asm volatile ("dmb" : : : "memory");
}

/**
 * Data Synchronization Barrier
 */
static inline void dsb(void) {
    asm volatile ("dsb" : : : "memory");
}


/**
 * TTBCR configuration helpers
 */
static inline void ttbcr_set_split(uint32_t n) {
    uint32_t val = get_ttbcr();
    val &= ~TTBCR_N_MASK;  // Clear N bits
    val |= (n & TTBCR_N_MASK);  // Set new N value
    set_ttbcr(val);
}

static inline uint32_t ttbcr_get_split(void) {
    return get_ttbcr() & TTBCR_N_MASK;
}

/* Configure 2GB/2GB split (TTBR0: 0x00000000-0x7FFFFFFF, TTBR1: 0x80000000-0xFFFFFFFF) */
static inline void ttbcr_configure_2gb_split(void) {
    ttbcr_set_split(TTBCR_SPLIT_2GB);
}

/* Enable/Disable TTBR0 or TTBR1 */
static inline void ttbcr_disable_ttbr0(void) {
    uint32_t val = get_ttbcr();
    val |= TTBCR_PD0_MASK;
    set_ttbcr(val);
}

static inline void ttbcr_enable_ttbr0(void) {
    uint32_t val = get_ttbcr();
    val &= ~TTBCR_PD0_MASK;
    set_ttbcr(val);
}

static inline void ttbcr_disable_ttbr1(void) {
    uint32_t val = get_ttbcr();
    val |= TTBCR_PD1_MASK;
    set_ttbcr(val);
}

static inline void ttbcr_enable_ttbr1(void) {
    uint32_t val = get_ttbcr();
    val &= ~TTBCR_PD1_MASK;
    set_ttbcr(val);
}

/* Check which TTBR handles a specific address */
static inline int is_ttbr0_addr(uint32_t addr, uint32_t n) {
    return addr <= TTBR0_BOUNDARY(n);
}

static inline int is_ttbr1_addr(uint32_t addr, uint32_t n) {
    return addr > TTBR0_BOUNDARY(n);
}


typedef uint32_t l2_page_table_t[256];
// // L1 Page Table (4096 entries, 4KB each for 4GB address space)
#ifdef PLATFORM_BBB
extern volatile uint32_t * const l1_page_table;
extern l2_page_table_t * const l2_tables;
#else
extern uint32_t l1_page_table[4096] __attribute__((aligned(16384)));
extern l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));
#endif

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

    // on arm, these do different things. on x86, they are the same.
    void (*set_kernel_table)(uint32_t* l1_table);
    void (*set_user_table)(uint32_t* l1_table);

    // checks if the address is in the user space for current_process.
    int (*is_user_addr)(uint32_t addr, uint32_t n);

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
    //
    void (*map_hardware_pages)(void); // implementation specific function to map hardware pages.

    uint32_t kernel_mem; // if the high kernel memory space dram is used, if 0, physical memory is used.
} mmu_t;

extern mmu_t mmu_driver;

#endif // KERNEL_MMU_H
