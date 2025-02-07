#include <kernel/syscall.h>
#include <kernel/printk.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/string.h>
#include <kernel/heap.h>
#include <kernel/sched.h>
#include <kernel/paging.h>

int sys_getpid(void) {
    return current_process->pid;
}

int sys_yield(void) {
    scheduler();
    return -1; // should never reach here
}


int sys_debug(int buf, int len) {
    char* buff = kmalloc(len);
    if (buff == NULL) {
        return -1;
    }

    memcpy(buff, (uint8_t*)buf, len);

    uint32_t process_table = (uint32_t)mmu_driver.ttbr0;
    // switch page taable so we can print, we won't want this later

    uint32_t kernel_l1_phys = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    mmu_driver.set_l1_table((uint32_t*) kernel_l1_phys);

    printk("%s\n", buff);

    mmu_driver.set_l1_table((uint32_t*) process_table);
    return 0;
}

int sys_exit(int exit_status) {
    // free allocated pages
    uint32_t kernel_l1_phys = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    mmu_driver.set_l1_table((uint32_t*)kernel_l1_phys);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->code_page_vaddr);
    mmu_driver.unmap_page((void*)current_process->ttbr0, (void*)current_process->data_page_vaddr);
    free_page(&kpage_allocator, (void*)current_process->code_page_paddr);
    free_page(&kpage_allocator, (void*)current_process->data_page_paddr);
    // TODO free pages in l1 table of process:
    // TODO free anything else in the process
    // TODO reassign PID or parents of children
    free_alligned_pages(&kpage_allocator, current_process->ttbr0, 4);
    memset(current_process, 0, sizeof(*current_process));


    scheduler(); // reschedule after releasing the process struct;
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
};

int handle_syscall(int num, int arg1, int arg2, int arg3, int arg4) {
    if (num >= 0 && num < NR_SYSCALLS) {
        switch(syscall_table[num].num_args) {
            case 0: return syscall_table[num].fn.fn0();
            case 1: return syscall_table[num].fn.fn1(arg1);
            case 2: return syscall_table[num].fn.fn2(arg1, arg2);
            case 3: return syscall_table[num].fn.fn3(arg1, arg2, arg3);
            case 4: return syscall_table[num].fn.fn4(arg1, arg2, arg3, arg4);
        }
    }
    return -1;
}
