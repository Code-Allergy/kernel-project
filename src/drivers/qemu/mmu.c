#include <stdint.h>

// Align to 16KB (ARMv7 requirement for TTBR0)
#define MMU_TABLE_ALIGN  __attribute__((aligned(16384)))

#define MMU_SECTION_DESCRIPTOR (2 << 0)     // Section descriptor (b10)
#define MMU_DOMAIN_KERNEL      0            // Domain 0 for kernel
#define MMU_AP_RW              (3 << 10)    // Read/write access
#define MMU_CACHEABLE          (1 << 3)     // Enable caching
#define MMU_BUFFERABLE         (1 << 2)     // Enable write buffering

// L1 Page Table (4096 entries, 4KB each for 4GB address space)
static uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;