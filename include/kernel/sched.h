#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H
#include <kernel/vfs.h>
#include <kernel/file.h>
#include <kernel/list.h>
#include <elf32.h>

#include <stdint.h>

#define MAX_PROCESSES 128
#define MAX_ASID 255  // ARMv7 supports 8-bit ASIDs (0-255)
#define NULL_PROCESS_FILE "/elf/null"
#define INIT_PROCESS_FILE "/elf/init"

/* Process state definitions */
#define PROCESS_RUNNING  1
#define PROCESS_KILLED   2
#define PROCESS_READY    3
#define PROCESS_BLOCKED  4
#define PROCESS_SLEEPING 5
#define PROCESS_UNINTERUPTABLE 6
#define PROCESS_NONE     0

/* Kernel ticks until scheduler force reschedules */
#define SCHEDULER_PREEMPT_TICKS 2 // should be a power of 2 ideally for faster modulo

struct cpu_regs {
    uint32_t r0;  // 0
    uint32_t r1;  // 4
    uint32_t r2;  // 8
    uint32_t r3;  // 12
    uint32_t r4;  // 16
    uint32_t r5;  // 20
    uint32_t r6;  // 24
    uint32_t r7;  // 28
    uint32_t r8;  // 32
    uint32_t r9;  // 36
    uint32_t r10; // 40
    uint32_t r11; // 44
    uint32_t r12; // 48
    uint32_t sp;  // 52
    uint32_t lr;  // 56
    uint32_t cpsr;// 60
    uint32_t pc;  // 64
};

enum process_page_type {
    PROCESS_PAGE_CODE,
    PROCESS_PAGE_DATA,
    PROCESS_PAGE_STACK,
    PROCESS_PAGE_HEAP,
    PROCESS_PAGE_OTHER
};

typedef struct process_page_ref {
    struct list_head list;
    struct process_page* page;
} process_page_ref_t;

typedef struct process_page {
    void* vaddr;           // Virtual address of the page
    void* paddr;           // Physical address of the page
    uint32_t ref_count;    // Number of processes referencing this page
    uint32_t flags;        // Page flags (read/write/execute etc)

    enum process_page_type page_type;   // Type of page (code, data, stack etc)
    // size_t size;           // Size of the page (all 4k for now)
    // uint32_t last_access;  // Timestamp of last page access
} process_page_t;


typedef struct process_struct {
    uint32_t* stack_top;
    uint32_t* stack_base_paddr;
    int32_t pid;
    int32_t ppid;
    uint32_t priority;
    uint32_t state;

    uint64_t wake_ticks;   // sleep state wake time
    struct list_head list; // for wait queue
    void* blocked_on;      // pointer to the object the process is blocked on

    // Memory management
    uint32_t* ttbr0;      // Physical address of translation table base
    uint32_t asid;        // Address Space ID

    uint32_t code_size;
    uint32_t code_entry;

    struct list_head pages_head;
    uint32_t num_pages;

    // file management
    vfs_file_t* fd_table[MAX_FDS];
    int num_fds;

    int forked; // 1 if forked, 0 if not -- DEBUG - remove later

    uint32_t syscall_trace; // syscalls to trace (bitmask)
    int32_t exit_status;    // exit status of the process

    struct process_struct* waiting_parent;
    // debug info
    char* process_name;            // Name or identifier for the process
    uint32_t creation_time;        // Process creation timestamp
    uint32_t last_schedule_time;   // Last time this process was scheduled
    uint32_t total_run_time;       // Total CPU time used
    uint32_t context_switches;     // Number of times process was context switched
    uint32_t page_faults;          // Number of page faults
    uint32_t syscall_count;        // Number of system calls made
    char* current_syscall;         // Name of currently executing syscall if any
    uint32_t stack_usage;          // Current stack usage
    uint32_t heap_usage;          // Current heap memory usage
} process_t;
extern process_t* current_process;

typedef struct {
    int schedule_next;
    process_t* current_process;

    int num_processes;
    int max_processes;

    uint32_t current_tick;

    void (*tick)(void);
} scheduler_t;
extern scheduler_t scheduler_driver;

// get the process by pid
process_t* get_process_by_pid(int32_t pid);

// this can create or fork a process, based on which parameter is non-NULL
process_t* create_process(binary_t* bin, process_t* parent);

// swap the currently executing code in process with the code in bin, without destroying the process or it's members
int swap_process(binary_t* bin, process_t* process);

// specifically free only the memory pages of a process (for exec or for cleanup)
void free_process_memory(process_t* p);

// initialize the scheduler
int scheduler_init(void);

// run the scheduler and start the next process. This function should never return, so be mindful of the stack usage.
void scheduler(void) __attribute__ ((noreturn));

// call to jump to userspace, either back to current process or to a new process
void __attribute__((noreturn)) userspace_return(void);

#endif // KERNEL_SCHED_H
