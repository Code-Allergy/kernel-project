#include "kernel/panic.h"
#include <kernel/boot.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/mm.h>
#include <kernel/mmu.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/fat32.h>

static uint32_t total_processes;
static uint32_t curr_pid;
#define MAX_PROCESSES 16
#define MAX_ASID 255  // ARMv7 supports 8-bit ASIDs (0-255)

// use a bitmap and struct later for this and other flags
volatile bool schedule_needed = false;

process_t* current_process = NULL;
process_t* next_process = NULL;
process_t process_table[MAX_PROCESSES];
static uint8_t asid_bitmap[MAX_ASID + 1] = {0};

process_t* spawn_flat_init_process(const char* file_path) {
    fat32_fs_t sd_card;
    fat32_file_t userspace_application;
    if (fat32_mount(&sd_card, &mmc_fat32_diskio) != 0) {
        printk("Failed to mount SD card\n");
        return NULL;
    }
    if (fat32_open(&sd_card, file_path, &userspace_application)) {
        printk("Failed to open file\n");
        return NULL;
    }

    static uint8_t bytes[PAGE_SIZE];
    if ((uint32_t)fat32_read(&userspace_application, bytes, PAGE_SIZE) != userspace_application.file_size) {
        printk("Init process not read fully off disk\n");
        return NULL;
    };

    return create_process(bytes, userspace_application.file_size);
}

static int32_t get_next_pid(void) {
    printk("Allocating PID: %d\n", total_processes);
    return total_processes++;
}

// this shouldn't flush EL2, and shouldn't be here either. not sure why it's here
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
    current_process = NULL;
    total_processes = 0;
    curr_pid = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_NONE;
    }

    scheduler_driver.current_tick = 0;
    // spawn_flat_init_process("/bin/null");
    process_t* p = spawn_flat_init_process("/bin/open");
    // process_t* p_ = spawn_flat_init_process("/bin/testa");
    // spawn_flat_init_process("/bin/testa");
    // spawn_flat_init_process("/bin/testa");
    // spawn_flat_init_process("/bin/testa");
    // spawn_flat_init_process("/bin/testa");
    // spawn_flat_init_process("/bin/testa");

    // clone_process(p);
    // clone_process(p);
    // scheduler();
    return 0;
}

uint8_t allocate_asid(void) {
    for (uint32_t i = 1; i <= MAX_ASID; i++) {  // Skip ASID 0 (reserved)
        if (!asid_bitmap[i]) {
            asid_bitmap[i] = 1;
            return i;
        }
    }
    // If all ASIDs are used, flush TLB and reuse
    mmu_driver.flush_tlb();
    memset(asid_bitmap, 0, sizeof(asid_bitmap));
    asid_bitmap[1] = 1;
    return 1;
}

void copy_kernel_mappings(uint32_t *ttbr0) {
    uint32_t *kernel_ttbr0;
    // Read the current TTBR0 value (Kernel's TTBR0 mapping)
    __asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(kernel_ttbr0));

    // Start at the kernel entry point and copy mappings up to the kernel end
    for (uint32_t addr = 0x80000000; addr < 0xFFEFFFFF; addr += 0x100000) {
        // Compute the index into the TTBR0 table
        uint32_t index = addr / 0x100000;
        ttbr0[index] = kernel_ttbr0[index];
    }
}

// round robin scheduler
process_t* get_next_process(void) {
    uint32_t start;
    if (current_process == NULL) {
        printk("Current process is NULL\n");
        start = 0;
    } else {
        start = current_process->pid + 1 % MAX_PROCESSES;
    }
    uint32_t current = start;
    do {
        if (process_table[current].state == PROCESS_READY) {
            return &process_table[current];
        }
        current = (current + 1) % MAX_PROCESSES;
    } while (current != start);

    return current_process; // If no other process found, return current one
}
void __attribute__((noreturn, naked)) user_context_return(uint32_t stack_ptr);


void debug_stack_access(process_t* proc) {
    printk("Process %d stack info:\n", proc->pid);
    printk("  Stack base: %p\n", proc->stack_page_vaddr);
    printk("  Stack top: %p\n", proc->context.sp);
    printk("  Stack physical: %p\n", proc->stack_page_paddr);
}

void __attribute__ ((noreturn)) scheduler(void) {
    next_process = get_next_process();
    if (next_process == NULL || next_process->state == PROCESS_NONE) {
        panic("No more processes to run, halting!\n");
    }

    // printk("Starting process pid %u\n", next_process->pid);
    // printk("Jumping to code at %p\n", *(uint32_t*)(next_process->stack_page_paddr + PAGE_SIZE - (3 * sizeof(uint32_t))));
    // printk("Stack top: %p\n", next_process->stack_top);

    // debug_l1_l2_entries((void*)0x00010000, next_process->ttbr0);
    // printk("Phys=0x446A9000: %p", *(uint32_t*)0x446A9000);
    mmu_driver.set_l1_with_asid(next_process->ttbr0, next_process->asid);
    // Add TLB invalidation for the new ASID
    __asm__ volatile (
        "dsb ish\n"
        "mcr p15, 0, %0, c8, c7, 2\n"  // Invalidate TLB by ASID
        "dsb ish\n"
        "isb\n"
        : : "r" (next_process->asid)
    );

    current_process = next_process;
    scheduler_driver.schedule_next = 0;

    // fix the stack from the compiler function prologue
    __asm__ ("add sp, sp, #8\n"); // TODO this is a hack, fix it. this WILL break.
    // TO fix, we should have the scheduler setup in another function and return then pass to asm
    // this way we won't have to deal with the compiler's prologue

    // Direct tail call with forced assembly branch
    register uint32_t stack_ptr __asm__("r0") = (uint32_t)next_process->stack_top;
    __asm__ volatile(
        "bx %0"
        :
        : "r" (user_context_return), "r" (stack_ptr)
        : "memory"
    );
    __builtin_unreachable();
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

    // allocate pages for code and data
    void* data_page = alloc_page(&kpage_allocator);
    // TODO - frame corruption?? look into what's happening to mapping here
    void* stack_page = alloc_page(&kpage_allocator);
    void* heap_page = alloc_page(&kpage_allocator);
    if (!data_page || !stack_page || !heap_page) { // NO SWAPPING
        panic("Out of memory in clone_process! HALT\n");
    }

    p->ttbr0 = (uint32_t*) alloc_l1_table(&kpage_allocator);
    if(!p->ttbr0) return NULL;

    mmu_driver.map_page(p->ttbr0, (void*)MEMORY_USER_CODE_BASE, (void*)original_p->code_page_paddr, MMU_NORMAL_MEMORY | MMU_AP_RO | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    mmu_driver.map_page(p->ttbr0, (void*)MEMORY_USER_DATA_BASE, data_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    mmu_driver.map_page(p->ttbr0, (void*)MEMORY_USER_STACK_BASE, stack_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    mmu_driver.map_page(p->ttbr0, (void*)MEMORY_USER_HEAP_BASE, heap_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);

    p->asid = allocate_asid();
    p->pid = get_next_pid();
    p->ppid = original_p->pid;
    p->priority = original_p->priority;

    // map kernel mappings
    mmu_driver.set_l1_with_asid(p->ttbr0, p->asid);

    memcpy((void*)MEMORY_USER_DATA_BASE, PHYS_TO_KERNEL_VIRT(original_p->data_page_paddr), PAGE_SIZE); // TODO COW for data page
    memcpy((void*)MEMORY_USER_STACK_BASE, PHYS_TO_KERNEL_VIRT(original_p->stack_page_paddr), PAGE_SIZE); // TODO COW for stack page / only copy in use stack
    memcpy((void*)MEMORY_USER_HEAP_BASE, PHYS_TO_KERNEL_VIRT(original_p->heap_page_paddr), PAGE_SIZE); // TODO COW for heap page

    p->code_page_vaddr = MEMORY_USER_CODE_BASE;
    p->stack_page_vaddr = MEMORY_USER_STACK_BASE;
    p->data_page_vaddr = MEMORY_USER_DATA_BASE;
    p->heap_page_vaddr = MEMORY_USER_HEAP_BASE;

    p->code_page_paddr = (uint32_t) original_p->code_page_paddr;
    p->data_page_paddr = (uint32_t) data_page;
    p->stack_page_paddr = (uint32_t) stack_page;
    p->heap_page_paddr = (uint32_t) heap_page;

    p->stack_top = original_p->stack_top;
    p->code_size = original_p->code_size;
    p->state = PROCESS_READY;

    // Debug: Print L1 entry for user stack
    // uint32_t stack_vaddr = MEMORY_USER_STACK_BASE;
    // uint32_t l1_index = stack_vaddr >> 20;
    // uint32_t l1_entry = p->ttbr0[l1_index];
    // printk("Clone L1[%d] = 0x%08x (phys: 0x%08x)\n",
    //        l1_index, l1_entry, (l1_entry & 0xFFF00000));

    // printk("stack paddr: %p\nheap paddr: %p\n", stack_page, heap_page);

    // uint32_t* addr = mmu_driver.get_physical_address(p->ttbr0, (void*)stack_vaddr);
    // printk("Physical address1:%p\n", addr);

    // uint32_t* addr2 = mmu_driver.get_physical_address(p->ttbr0, (void*)MEMORY_USER_HEAP_BASE);
    // printk("Physical address2:%p\n", addr2);

    return p;
}


// create a new process with a flat binary
process_t* create_process(uint8_t* bytes, size_t size) {
    // printk("Process bytes: %p %p %p %p\n", *(uint32_t*)bytes, *(uint32_t*)bytes + 4, *(uint32_t*)bytes + 8, *(uint32_t*)bytes + 12);
    process_t* proc = get_available_process();
    if (proc == NULL) {
        printk("Out of available processes in create_process! HALT\n");
        while (1);
    }

    void* code_page = alloc_page(&kpage_allocator);
    void* data_page = alloc_page(&kpage_allocator);
    void* stack_page = alloc_page(&kpage_allocator);
    void* heap_page = alloc_page(&kpage_allocator);
    if (!code_page || !data_page || !stack_page || !heap_page) { // NO SWAPPING
        printk("Out of memory in create_process! HALT\n");
        while (1);
    }

    // Allocate 16KB-aligned L1 table
    proc->ttbr0 = (uint32_t*) alloc_l1_table(&kpage_allocator);
    if(!proc->ttbr0) return NULL;

    mmu_driver.map_page(proc->ttbr0, (void*)MEMORY_USER_CODE_BASE, code_page, MMU_NORMAL_MEMORY | MMU_AP_RO | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    mmu_driver.map_page(proc->ttbr0, (void*)MEMORY_USER_DATA_BASE, data_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    mmu_driver.map_page(proc->ttbr0, (void*)MEMORY_USER_STACK_BASE, stack_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    mmu_driver.map_page(proc->ttbr0, (void*)MEMORY_USER_HEAP_BASE, heap_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    memcpy(PHYS_TO_KERNEL_VIRT(code_page), bytes, size);


    proc->asid = allocate_asid();
    proc->pid = get_next_pid();
    proc->priority = 0;

    proc->code_page_vaddr = MEMORY_USER_CODE_BASE;
    proc->data_page_vaddr = MEMORY_USER_DATA_BASE;
    proc->stack_page_vaddr = MEMORY_USER_STACK_BASE;
    proc->heap_page_vaddr = MEMORY_USER_HEAP_BASE;

    proc->code_page_paddr = (uint32_t) code_page;
    proc->data_page_paddr = (uint32_t) data_page;
    proc->stack_page_paddr = (uint32_t) stack_page;
    proc->heap_page_paddr = (uint32_t) heap_page;
    proc->code_size = size;

    proc->num_fds = 0;

    mmu_driver.set_l1_with_asid(proc->ttbr0, proc->asid);
    proc->stack_top = (uint32_t*)(MEMORY_USER_STACK_BASE + PAGE_SIZE - (16 * sizeof(uint32_t)));

    proc->stack_top[13] = MEMORY_USER_CODE_BASE; // lr -- TODO: set to exit handler backup
    proc->stack_top[14] = 0x10; // cpsr
    proc->stack_top[15] = MEMORY_USER_CODE_BASE; // pc

    proc->state = PROCESS_READY;
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

void tick(void) {
    if (scheduler_driver.current_tick++ % SCHEDULER_PREEMPT_TICKS == 0) {
        scheduler_driver.schedule_next = 1;
    }
}

void mmu_set_l1_with_asid(uint32_t ttbr0, uint32_t asid) {
    mmu_driver.set_l1_with_asid((uint32_t*)ttbr0, asid);
}


// TODO clean this up
__attribute__((naked, noreturn)) void userspace_return() {
    __asm__ volatile(
        "ldr r3, =scheduler_driver\n\t"
        "ldr r3, [r3]\n\t"
        "cmp r3, #0\n\t"
        "beq 1f\n\t"
        "b scheduler\n"
        "1:\n\t"
        "ldr r3, =current_process\n\t"
        "ldr r3, [r3]\n\t"
        "ldr r0, [r3, %[ttbr0_off]]\n\t"
        "ldr r1, [r3, %[asid_off]]\n\t"
        "bl mmu_set_l1_with_asid\n\t"
        "ldr r3, =current_process\n\t"
        "ldr r3, [r3]\n\t"
        "ldr r0, [r3, %[stack_off]]\n\t"
        "b user_context_return\n\t"
        :
        : [ttbr0_off] "i" (offsetof(process_t, ttbr0)),
          [asid_off] "i" (offsetof(process_t, asid)),
          [stack_off] "i" (offsetof(process_t, stack_top))
    );
}

scheduler_t scheduler_driver = {
    .schedule_next = 0,
    .tick = tick,
};
