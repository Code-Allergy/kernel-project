#include <stdint.h>
#include <kernel/mm.h>
#include <kernel/mmu.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/printk.h>
#include <kernel/string.h>

#define MAX_PROCESSES 16

process_t* current_process = NULL;
process_t process_table[MAX_PROCESSES];

#define MAX_ASID 255  // ARMv7 supports 8-bit ASIDs (0-255)
static uint8_t asid_bitmap[MAX_ASID + 1] = {0};

static inline void flush_tlb(void) {
    __asm__ volatile (
        "dsb ish\n"        // Ensure all previous memory accesses complete
        "tlbi alle2\n"     // Invalidate all TLB entries at EL2
        "dsb ish\n"        // Ensure completion of TLB invalidation
        "isb\n"            // Synchronize the instruction stream
        : // No output operands
        : // No input operands
        : "memory"         // Clobbers memory
    );
}

// never return
int scheduler_init(void) {
    printk("No scheduler implemented, halting!\n");
    while(1) {
        __asm__ volatile("wfi");
    }
}

uint8_t allocate_asid(void) {
    for (uint8_t i = 1; i <= MAX_ASID; i++) {  // Skip ASID 0 (reserved)
        if (!asid_bitmap[i]) {
            asid_bitmap[i] = 1;
            return i;
        }
    }
    // If all ASIDs are used, flush TLB and reuse

    flush_tlb();
    memset(asid_bitmap, 0, sizeof(asid_bitmap));
    asid_bitmap[1] = 1;
    return 1;
}

// void copy_kernel_mappings(uint32_t *ttbr0) {
//     // Get kernel's TTBR1 table (assumes identity mapped)
//     uint32_t *kernel_ttbr1;
//     __asm__ volatile("mrc p15, 0, %0, c2, c0, 1" : "=r"(kernel_ttbr1));

//     // Copy upper half (kernel space) mappings
//     for(int i=2048; i<4096; i++) {  // Entries 2048-4096 = 0x80000000+
//         ttbr0[i] = kernel_ttbr1[i];
//     }
// }

void copy_kernel_mappings(uint32_t *ttbr1) {
    uint32_t *kernel_ttbr0;
    __asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(kernel_ttbr0));

    // Kernel at 0x40120000
    // 0x40120000 >> 20 = 1025 (entry number for start of kernel)
    const int KERNEL_START_ENTRY = 1024;  // Entry for 0x40120000
    const int KERNEL_SIZE_ENTRIES = 32;   // Adjust based on kernel size

    for(int i = KERNEL_START_ENTRY; i < KERNEL_START_ENTRY + KERNEL_SIZE_ENTRIES; i++) {
        ttbr1[i] = kernel_ttbr0[i];
    }
}


void schedule(void) {
    // Your scheduling logic here
    // For example: Round Robin implementation
    static int last_pid = 0;

    do {
        last_pid = (last_pid + 1) % MAX_PROCESSES;
    } while(process_table[last_pid].state != PROCESS_READY);

    current_process = &process_table[last_pid];

    // Perform actual context switch (you'll need assembly for this)
    context_switch(&current_process->regs);
}


process_t* create_process(void* code_page, void* data_page, uint8_t* bytes, size_t size) {
    process_t* proc = &process_table[0];

    // Allocate 16KB-aligned L1 table
    proc->ttbr0 = (uint32_t*) alloc_l1_table(&kpage_allocator);
    if(!proc->ttbr0) return NULL;

    map_page(proc->ttbr0, (void*)MEMORY_USER_CODE_BASE, code_page, MMU_NORMAL_MEMORY | MMU_AP_RO | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    map_page(proc->ttbr0, (void*)MEMORY_USER_DATA_BASE, data_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    memcpy(code_page, bytes, size);
    // uint32_t l1_index = MEMORY_USER_CODE_BASE >> 20;
    // proc->ttbr1[l1_index] = (uint32_t)alloc_page(&kpage_allocator) | MMU_PAGE_DESCRIPTOR;
    // proc->ttbr1[l1_index] |= 1 << 5;
    // uint32_t *l2_table = (uint32_t*)(proc->ttbr1[l1_index] & ~0x3FF);
    // l2_table[PAGE_INDEX(MEMORY_USER_CODE_BASE)] = (uint32_t)code_page | L2_SMALL_PAGE | MMU_AP_RO;

    // Copy kernel mappings (TTBR1 content)
    printk("Kernel mappings=precopied\n");
    //
    copy_kernel_mappings(proc->ttbr0);
    printk("Kernel mappings copied\n");
    // Set ASID
    proc->asid = allocate_asid();
    proc->pid = 0;
    proc->priority = 0;
    proc->state = 0;

    proc->code_page = MEMORY_USER_CODE_BASE;
    proc->data_page = MEMORY_USER_DATA_BASE;

    proc->regs.r0 = 0;
    proc->regs.r1 = 0;
    proc->regs.r2 = 0;
    proc->regs.r3 = 0;
    proc->regs.r4 = 0;
    proc->regs.r5 = 0;
    proc->regs.r6 = 0;
    proc->regs.r7 = 0;
    proc->regs.r8 = 0;
    proc->regs.r9 = 0;
    proc->regs.r10 = 0;
    proc->regs.r11 = 0;
    proc->regs.r12 = 0;
    proc->regs.sp = MEMORY_USER_STACK_BASE;
    proc->regs.pc = MEMORY_USER_CODE_BASE;
    proc->regs.cpsr = 0x10; // user mode

    // TODO
    proc->regs.lr = 0;
    printk("returning process\n");

    return proc;
}

void get_kernel_regs(struct cpu_regs* regs) {
    __asm__ volatile("mrs %0, cpsr" : "=r"(regs->cpsr));
    __asm__ volatile("mov %0, r0" : "=r"(regs->r0));
    __asm__ volatile("mov %0, r1" : "=r"(regs->r1));
    __asm__ volatile("mov %0, r2" : "=r"(regs->r2));
    __asm__ volatile("mov %0, r3" : "=r"(regs->r3));
    __asm__ volatile("mov %0, r4" : "=r"(regs->r4));
    __asm__ volatile("mov %0, r5" : "=r"(regs->r5));
    __asm__ volatile("mov %0, r6" : "=r"(regs->r6));
    __asm__ volatile("mov %0, r7" : "=r"(regs->r7));
    __asm__ volatile("mov %0, r8" : "=r"(regs->r8));
    __asm__ volatile("mov %0, r9" : "=r"(regs->r9));
    __asm__ volatile("mov %0, r10" : "=r"(regs->r10));
    __asm__ volatile("mov %0, r11" : "=r"(regs->r11));
    __asm__ volatile("mov %0, r12" : "=r"(regs->r12));
    __asm__ volatile("mov %0, sp" : "=r"(regs->sp));
    __asm__ volatile("mov %0, lr" : "=r"(regs->lr));
    __asm__ volatile("mov %0, pc" : "=r"(regs->pc));
}
