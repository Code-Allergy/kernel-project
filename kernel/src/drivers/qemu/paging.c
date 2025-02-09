#include <kernel/boot.h>
#include <kernel/printk.h>
// #include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>
#include <kernel/string.h>

extern uint32_t kernel_code_end; // Defined in linker script, end of kernel code space, pages should be RO
extern uint32_t kernel_end; // Defined in linker script, end of kernel memory space
#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)
