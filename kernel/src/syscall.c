#include <kernel/syscall.h>
#include <kernel/printk.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/string.h>
#include <kernel/heap.h>


typedef int (*syscall_fn_0)(void);
typedef int (*syscall_fn_1)(int);
typedef int (*syscall_fn_2)(int, int);
typedef int (*syscall_fn_3)(int, int, int);
typedef int (*syscall_fn_4)(int, int, int, int);

typedef union {
    syscall_fn_0 fn0;
    syscall_fn_1 fn1;
    syscall_fn_2 fn2;
    syscall_fn_3 fn3;
    syscall_fn_4 fn4;
} syscall_fn;

enum syscall_num {
    SYS_DEBUG = 0,
    // SYS_EXIT = 1,
};

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




static const struct {
    syscall_fn fn;
    const char *name;
    int num_args;
} syscall_table[NR_SYSCALLS] = {
    [SYS_DEBUG]  = {{.fn2 = sys_debug},  "debug",  2},
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
