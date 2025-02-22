#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

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

#define NR_SYSCALLS 13
enum syscall_num {
    SYS_DEBUG         = 0,
    SYS_EXIT          = 1,
    SYS_GETPID        = 2,
    SYS_YIELD         = 3,
    SYS_OPEN          = 4,
    SYS_CLOSE         = 5,
    SYS_FORK          = 6,
    SYS_READDIR       = 7,
    SYS_READ          = 8,
    SYS_WRITE         = 9,
    SYS_EXEC          = 10,
    SYS_TIME          = 11,
    SYS_GETTIMEOFDAY  = 12,
    SYS_USLEEP        = 13,
};

typedef struct syscall_entry {
    syscall_fn fn;
    const char *name;
    int num_args;
} syscall_entry_t;

extern const syscall_entry_t syscall_table[NR_SYSCALLS + 1];

// Called by SVC handler in int.c
int handle_syscall(int num, int arg1, int arg2, int arg3, int arg4);
const char* syscall_get_name(int num);
int syscall_get_num_args(int num);

#endif
