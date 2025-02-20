#include <kernel/vfs.h>
#include <kernel/syscall.h>
#include <kernel/printk.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/string.h>
#include <kernel/heap.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/panic.h>

#define __user

// here for now, later we can move this
// copy len bytes to current_process dest from src
int copy_to_user(uint8_t* __user dest, const uint8_t* src, size_t len) {
    if (!mmu_driver.is_user_addr((uint32_t) dest, len)) {
        panic("failed tto check if user addr");
        return -1;
    }

    // TODO: any other checks? we don't need to copy to kvirt memory, because of ttbr0/ttbr1
    memcpy(dest, src, len);
    return 0;
}

int copy_from_user(uint8_t* dest, const uint8_t* __user src, size_t len) {
    if (!mmu_driver.is_user_addr((uint32_t) src, len)) {
        panic("failed to check if user addr");
        return -1;
    }

    memcpy(dest, src, len);
    return 0;
}

// doesn't work, clone_process does not function properly and switching to a second process breaks this version of the kernel
int sys_fork(void) {
    process_t* child = clone_process(current_process);
    child->stack_top[0] = child->pid; // return value of fork in child is the pid
    mmu_driver.set_l1_with_asid(current_process->ttbr0, current_process->asid);

    return 0;
}

int sys_getpid(void) {
    return current_process->pid;
}

int sys_yield(void) {
    scheduler_driver.schedule_next = 1;
    return 0;
}

int sys_open(char* path, int flags, int mode) {
    // for now, just check if we are opening root, but use copy_from_user later
    int res = -1;
    if (strcmp(path, "/") == 0) {
        res = vfs_root_node->ops->open(vfs_root_node, flags);
    }

    return res;
}

int sys_close(int fd) {
    int res = vfs_root_node->ops->close(fd);

    return res;
}

// this should copy memory instead
int sys_debug(int buf, int len) {
    // TODO: we should still copy from user space, or maybe we don't need to?
    // after we have proper domain and ttbr1 setup, we can just use the user space pointer, as long as we check if it's valid
    printk("[SYS_DEBUG]: %s", buf);
    return 0;
    // syscall_return(&current_process->context, 0);
}

int sys_exit(int exit_status) {
    // mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->code_page_vaddr); // Need RCs before we can free this, for now just let it leak
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->data_page_vaddr);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->stack_page_vaddr);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->heap_page_vaddr);
    // free_page(&kpage_allocator, (void*)current_process->code_page_paddr); // Need RCs before we can free this, for now just let it leak
    free_page(&kpage_allocator, (void*)current_process->data_page_paddr);
    free_page(&kpage_allocator, (void*)current_process->stack_page_paddr);
    free_page(&kpage_allocator, (void*)current_process->heap_page_paddr);


    // TODO free pages in l1 table of process:
    // TODO free anything else in the process
    // TODO reassign PID or parents of children
    // TODO clean up any open files
    free_aligned_pages(&kpage_allocator, current_process->ttbr0, 4);
    memset(current_process, 0, sizeof(process_t));
    current_process->state = PROCESS_NONE;
    current_process = NULL;
    scheduler_driver.schedule_next = 1;
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
    [SYS_CLOSE]  = {{.fn1 = sys_close},  "close",  1},
    [SYS_FORK]   = {{.fn0 = sys_fork},   "fork",   0},
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
    // Switch to kernel page table
    // uint32_t kernel_l1_table = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    // uint32_t process_table = (uint32_t)current_process->ttbr0;
    // mmu_driver.set_l1_table((uint32_t*)kernel_l1_table);

    // printk("Syscall num: %d, stackp: %p\n", num, arg1);

    // log the syscall
#ifdef TRACE_SYSCALLS
    switch (syscall_table[num].num_args) {
        case 0: printk("Syscall: %s()\n", syscall_table[num].name); break;
        case 1: printk("Syscall: %s(%d (%p))\n", syscall_table[num].name, arg1, arg1); break;
        case 2: printk("Syscall: %s(%d (%p), %d (%p))\n", syscall_table[num].name, arg1, arg1, arg2, arg2); break;
        case 3: printk("Syscall: %s(%d (%p), %d (%p), %d (%p))\n", syscall_table[num].name, arg1, arg1, arg2, arg2, arg3, arg3); break;
    }
#endif
    // printk("Stack pointer: %p\n", stack_pointer);
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
    // mmu_driver.set_l1_table((uint32_t*)process_table);
    // set up return value at r0


    // process doesn't necessarily exist anymore, so check for it first
    if (current_process) current_process->stack_top[0] = ret;
    return ret;
}
