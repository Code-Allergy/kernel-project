#include "kernel/vfs.h"
#include <kernel/syscall.h>
#include <kernel/printk.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/string.h>
#include <kernel/heap.h>
#include <kernel/sched.h>
#include <kernel/paging.h>


extern void syscall_return(struct cpu_regs*, int return_value);
// doesn't work, clone_process does not function properly and switching to a second process breaks this version of the kernel
int sys_fork(void) {
    process_t* child = clone_process(current_process);
    // child->context.r0 = 0; // return 0 for child
    // current_process->context.r0 = child->pid; // return child pid for parent
    scheduler(); // reschedule after creating a fork
}

int sys_getpid(void) {
    return current_process->pid;
}

int sys_yield(void) {
    scheduler_driver.schedule_next = 1;
    return 0;
}

int sys_open(char* path, int flags, int mode) {
    // for now, just check if we are opening root
    int res = -1;
    if (strcmp(path, "/") == 0) {
        res = vfs_root_node->ops->open(vfs_root_node, flags);
    }

    return res;
}


// this should copy memory instead
int sys_debug(int buf, int len) {
    uint32_t process_table = (uint32_t)mmu_driver.ttbr0;
    // switch page taable so we can print, we won't want this later

    uint32_t kernel_l1_phys = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    mmu_driver.set_l1_table((uint32_t*) kernel_l1_phys);
    void* paddr = mmu_driver.get_physical_address(current_process->ttbr0, (void*)buf);

    printk("[SYS_DEBUG]: %s\n", current_process->code_page_paddr + (uint32_t)buf - 0x10000);

    mmu_driver.set_l1_table((uint32_t*) process_table);
    return 0;
    // syscall_return(&current_process->context, 0);
}

int sys_exit(int exit_status) {
    uint32_t kernel_l1_phys = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    mmu_driver.set_l1_table((uint32_t*)kernel_l1_phys);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->code_page_vaddr);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->data_page_vaddr);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->stack_page_vaddr);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->heap_page_vaddr);
    free_page(&kpage_allocator, (void*)current_process->code_page_paddr);
    free_page(&kpage_allocator, (void*)current_process->data_page_paddr);
    free_page(&kpage_allocator, (void*)current_process->stack_page_paddr);
    free_page(&kpage_allocator, (void*)current_process->heap_page_paddr);


    // TODO free pages in l1 table of process:
    // TODO free anything else in the process
    // TODO reassign PID or parents of children
    free_aligned_pages(&kpage_allocator, current_process->ttbr0, 4);
    memset(current_process, 0, sizeof(*current_process));
    current_process->state = PROCESS_NONE;
    current_process = NULL;
    scheduler(); // reschedule after releasing the process struct;
}

static inline void dump_registers(struct cpu_regs* regs) {
    __asm__ volatile(
        "stmia %0, {r0-r12}\n"       // Save R0-R12 at offsets 0-48
        "str sp, [%0, #52]\n"        // Save SP
        "str lr, [%0, #56]\n"        // Save LR
        "mrs r1, cpsr\n"             // Get CPSR
        "str r1, [%0, #60]\n"        // Save CPSR
        "str lr, [%0, #64]\n"        // Save PC as LR (return address)
        :
        : "r"(regs)
        : "r1", "memory"             // Clobber r1 and inform about memory changes
    );
}




static const struct {
    syscall_fn fn;
    const char *name;
    int num_args;
} syscall_table[NR_SYSCALLS] = {
    [SYS_DEBUG]  = {{.fn2 = sys_debug},  "debug",  2},
    [SYS_EXIT]   = {{.fn1 = sys_exit},   "exit",   1},
    [SYS_GETPID] = {{.fn0 = sys_getpid}, "getpid", 0},
    [SYS_YIELD]  = {{.fn0 = sys_yield},  "yield",  0},
    [SYS_OPEN]   = {{.fn3 = sys_open},   "open",   3},
};

static inline void save_kernel_context(void) {
    uint32_t cpsr;
    __asm__ volatile(
        "mrs %0, cpsr\n"
        : "=r" (cpsr)
    );
    // Save current mode and state
    current_process->kernel_cpsr = cpsr;
}

static inline void restore_user_context(void) {
    uint32_t cpsr = current_process->context.cpsr;
    // Ensure we're returning to user mode
    cpsr &= ~0x1F;  // Clear mode bits
    cpsr |= 0x10;   // Set user mode
    cpsr &= ~0x80;  // Enable interrupts

    __asm__ volatile(
        "msr spsr_cxsf, %0\n"    // Set up spsr for return
        "mov lr, %1\n"           // Set up return address
        :
        : "r" (cpsr), "r" (current_process->context.lr)
        : "lr"
    );
}

int handle_syscall(int num, int arg1, int arg2, int arg3, int arg4, int stack_pointer) {
    // save_kernel_context();
    current_process->stack_top = stack_pointer;

    // Switch to kernel page table
    uint32_t kernel_l1_table = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    uint32_t process_table = (uint32_t)current_process->ttbr0;
    mmu_driver.set_l1_table((uint32_t*)kernel_l1_table);

    printk("Syscall: %s(%d, %d, %d, %d)\n", syscall_table[num].name, arg1, arg2, arg3, arg4);
    printk("Stack pointer: %p\n", stack_pointer);
    int ret = -1;
    if (num >= 0 && num < NR_SYSCALLS) {
        switch(syscall_table[num].num_args) {
            case 0: ret = syscall_table[num].fn.fn0(); break;
            case 1: ret = syscall_table[num].fn.fn1(arg1); break;
            case 2: ret = syscall_table[num].fn.fn2(arg1, arg2); break;
            case 3: ret = syscall_table[num].fn.fn3(arg1, arg2, arg3); break;
        }
    }

    // Return to process page table
    mmu_driver.set_l1_table((uint32_t*)process_table);
    // set up return value

    // restore_user_context();
    return ret;
}


// int handle_syscall(int num, int arg1, int arg2, int arg3, int arg4, int return_address) {
//     dump_registers(&current_process->context);
//     current_process->context.lr = return_address;
//     uint32_t kernel_l1_table = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
//     mmu_driver.set_l1_table((uint32_t*)kernel_l1_table);
//     printk("Syscall: %s(%d, %d, %d, %d)\n", syscall_table[num].name, arg1, arg2, arg3, arg4);
//     mmu_driver.set_l1_table((uint32_t*)current_process->ttbr0);
//     if (num >= 0 && num < NR_SYSCALLS) {
//         switch(syscall_table[num].num_args) {
//             case 0: return syscall_table[num].fn.fn0();
//             case 1: return syscall_table[num].fn.fn1(arg1);
//             case 2: return syscall_table[num].fn.fn2(arg1, arg2);
//             case 3: return syscall_table[num].fn.fn3(arg1, arg2, arg3);
//         }
//     }
//     return -1;
// }
