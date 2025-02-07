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
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_NONE;
    }


    // make init process, make it ready to start, then we can start it in scheduler

    // can call schedule here
}

uint8_t allocate_asid(void) {
    printk("Allocating ASID\n");
    for (uint8_t i = 1; i <= MAX_ASID; i++) {  // Skip ASID 0 (reserved)
        if (!asid_bitmap[i]) {
            printk("Got asid\n");
            asid_bitmap[i] = 1;
            return i;
        }
    }
    // If all ASIDs are used, flush TLB and reuse

    flush_tlb();
    memset(asid_bitmap, 0, sizeof(asid_bitmap));
    asid_bitmap[1] = 1;
    printk("Got asid\n");
    return 1;
}

// void copy_kernel_mappings(uint32_t *ttbr0) {
//     uint32_t *kernel_ttbr0;
//     __asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(kernel_ttbr0));

//     // start at kernel start entry, copy until end of memory space
//     const int KERNEL_START_ENTRY = KERNEL_ENTRY / 0x400000;
//     const int KERNEL_SIZE_ENTRIES = (0xFFFFFFFF - KERNEL_ENTRY) / 0x400000;

//     for(int i = KERNEL_START_ENTRY; i < KERNEL_START_ENTRY + KERNEL_SIZE_ENTRIES; i++) {
//         ttbr0[i] = kernel_ttbr0[i];
//     }
// }

void copy_kernel_mappings(uint32_t *ttbr0) {
    uint32_t *kernel_ttbr0;
    // Read the current TTBR0 value (Kernel's TTBR0 mapping)
    __asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(kernel_ttbr0));

    // Start at the kernel entry point and copy mappings up to the kernel end
    for (uint32_t addr = 0x80000000; addr < 0xF0000000; addr += 0x100000) {
        // Compute the index into the TTBR0 table
        uint32_t index = addr / 0x100000;
        ttbr0[index] = kernel_ttbr0[index];
    }
}

void scheduler(void) {
    // Your scheduling logic here
    // For example: Round Robin implementation
    // static int last_pid = 0;
    printk("Got to scheduler\n");
    while (1);
    // do {
    //     last_pid = (last_pid + 1) % MAX_PROCESSES;
    // } while(process_table[last_pid].state != PROCESS_READY);

    // current_process = &process_table[last_pid];

    // // Perform actual context switch (you'll need assembly for this)
    // context_switch(&current_process->context);
}

process_t* get_available_process(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_NONE) {
            return &process_table[i];
        }
    }
    return NULL;
}

process_t* clone_process(process_t* original_p) {
    process_t* p = get_available_process();
    if (p == NULL) {
        printk("Out of available processes in clone_process! HALT\n");
        while (1);
    }

    p->ttbr0 = (uint32_t*) alloc_l1_table(&kpage_allocator);
    if(!p->ttbr0) return NULL;
}

process_t* create_process(void* code_page, void* data_page, uint8_t* bytes, size_t size) {
    process_t* proc = get_available_process();
    if (proc == NULL) {
        printk("Out of available processes in create_process! HALT\n");
        while (1);
    }

    // Allocate 16KB-aligned L1 table
    proc->ttbr0 = (uint32_t*) alloc_l1_table(&kpage_allocator);
    if(!proc->ttbr0) return NULL;

    printk("Here\n");
    mmu_driver.map_page(proc->ttbr0, (void*)MEMORY_USER_CODE_BASE, code_page, MMU_NORMAL_MEMORY | MMU_AP_RO | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    printk("Page mapped\n");
    mmu_driver.map_page(proc->ttbr0, (void*)MEMORY_USER_DATA_BASE, data_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    printk("About to copy code page\n");
    memcpy(code_page, bytes, size);
    // uint32_t l1_index = MEMORY_USER_CODE_BASE >> 20;
    // proc->ttbr1[l1_index] = (uint32_t)alloc_page(&kpage_allocator) | MMU_PAGE_DESCRIPTOR;
    // proc->ttbr1[l1_index] |= 1 << 5;
    // uint32_t *l2_table = (uint32_t*)(proc->ttbr1[l1_index] & ~0x3FF);
    // l2_table[PAGE_INDEX(MEMORY_USER_CODE_BASE)] = (uint32_t)code_page | L2_SMALL_PAGE | MMU_AP_RO;

    // Copy kernel mappings (TTBR1 content)
    printk("Kernel mappings=precopied\n");
    copy_kernel_mappings(proc->ttbr0);
    printk("Kernel mappings copied to %p\n", proc->ttbr0);
    // Set ASID
    // proc->asid = allocate_asid();
    // TODO dynamic ASID allocation
    proc->asid = 1;
    proc->pid = 0;
    proc->priority = 0;
    proc->state = 0;

    proc->code_page_vaddr = MEMORY_USER_CODE_BASE;
    proc->data_page_vaddr = MEMORY_USER_DATA_BASE;
    proc->code_page_paddr = (uint32_t) code_page;
    proc->data_page_paddr = (uint32_t) data_page;

    proc->context.r0 = 1;
    proc->context.r1 = 2;
    proc->context.r2 = 3;
    proc->context.r3 = 4;
    proc->context.r4 = 5;
    proc->context.r5 = 6;
    proc->context.r6 = 7;
    proc->context.r7 = 8;
    proc->context.r8 = 9;
    proc->context.r9 = 10;
    proc->context.r10 = 11;
    proc->context.r11 = 12;
    proc->context.r12 = 13;
    proc->context.sp = MEMORY_USER_CODE_BASE + 0x500;
    proc->context.pc = MEMORY_USER_CODE_BASE;
    // proc->context.cpsr = 0x10; // user mode
    proc->context.cpsr = 0x50;

    // TODO
    proc->context.lr = 0;
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
