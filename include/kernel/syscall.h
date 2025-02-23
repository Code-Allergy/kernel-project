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

#define NR_SYSCALLS 14
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
    SYS_LSEEK         = 14,
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


// define macros to get around compiler warnings
#define END_SYSCALL }
#define DEFINE_SYSCALL0(name) \
    int sys_##name(void) {

#define DEFINE_SYSCALL1(name, t1, n1) \
    int sys_##name(int a1) { \
        t1 n1 = (t1)(uintptr_t)a1; (void)n1;

#define DEFINE_SYSCALL2(name, t1, n1, t2, n2) \
    int sys_##name(int a1, int a2) { \
        t1 n1 = (t1)(uintptr_t)a1; \
        t2 n2 = (t2)a2; (void)n1; (void)n2;

#define DEFINE_SYSCALL3(name, t1, n1, t2, n2, t3, n3) \
    int sys_##name(int a1, int a2, int a3) { \
        t1 n1 = (t1)(uintptr_t)a1; \
        t2 n2 = (t2)a2; \
        t3 n3 = (t3)a3; (void)n1; (void)n2; (void)n3;

#define DEFINE_SYSCALL4(name, t1, n1, t2, n2, t3, n3, t4, n4) \
    int sys_##name(int a1, int a2, int a3, int a4) { \
        t1 n1 = (t1)(uintptr_t)a1; \
        t2 n2 = (t2)a2; \
        t3 n3 = (t3)a3; \
        t4 n4 = (t4)a4; (void)n1; (void)n2; (void)n3; (void)n4;

#endif
