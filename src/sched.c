#include <stdint.h>
#include <kernel/mm.h>
#include <kernel/mmu.h>
#include <kernel/sched.h>
#include <kernel/paging.h>

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
int scheduler_init() {
    printk("No scheduler implemented, halting!\n");
    while(1) {
        asm volatile("wfi");
    }
}

uint8_t allocate_asid() {
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

void copy_kernel_mappings(uint32_t *ttbr0) {
    uint32_t *kernel_ttbr1;
    __asm__ volatile("mrc p15, 0, %0, c2, c0, 1" : "=r"(kernel_ttbr1));
    
    // Kernel at 0x40120000
    // 0x40120000 >> 20 = 1025 (entry number for start of kernel)
    const int KERNEL_START_ENTRY = 1025;  // Entry for 0x40120000
    const int KERNEL_SIZE_ENTRIES = 32;   // Adjust based on kernel size

    for(int i = KERNEL_START_ENTRY; i < KERNEL_START_ENTRY + KERNEL_SIZE_ENTRIES; i++) {
        ttbr0[i] = kernel_ttbr1[i];
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


process_t* create_process(uint32_t code_page, uint32_t data_page, uint32_t* bytes, uint32_t size) {
    process_t* proc = &process_table[0];

    // Allocate 16KB-aligned L1 table
    proc->ttbr0 = alloc_l1_table(&kpage_allocator);
    if(!proc->ttbr0) {
        return NULL;
    }

    proc->ttbr0[MEMORY_USER_CODE_BASE >> 20] = code_page | MMU_SECTION_DESCRIPTOR | MMU_AP_RO | MMU_CACHEABLE;

    // Copy kernel mappings (TTBR1 content)
    copy_kernel_mappings(proc->ttbr0);
    
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

    return proc;
}

void get_kernel_regs(struct cpu_regs* regs) {
    asm volatile("mrs %0, cpsr" : "=r"(regs->cpsr));
    asm volatile("mov %0, r0" : "=r"(regs->r0));
    asm volatile("mov %0, r1" : "=r"(regs->r1));
    asm volatile("mov %0, r2" : "=r"(regs->r2));
    asm volatile("mov %0, r3" : "=r"(regs->r3));
    asm volatile("mov %0, r4" : "=r"(regs->r4));
    asm volatile("mov %0, r5" : "=r"(regs->r5));
    asm volatile("mov %0, r6" : "=r"(regs->r6));
    asm volatile("mov %0, r7" : "=r"(regs->r7));
    asm volatile("mov %0, r8" : "=r"(regs->r8));
    asm volatile("mov %0, r9" : "=r"(regs->r9));
    asm volatile("mov %0, r10" : "=r"(regs->r10));
    asm volatile("mov %0, r11" : "=r"(regs->r11));
    asm volatile("mov %0, r12" : "=r"(regs->r12));
    asm volatile("mov %0, sp" : "=r"(regs->sp));
    asm volatile("mov %0, lr" : "=r"(regs->lr));
    asm volatile("mov %0, pc" : "=r"(regs->pc));
}
