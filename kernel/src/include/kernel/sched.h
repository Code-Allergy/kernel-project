#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <stddef.h>
#include <stdint.h>



/* Process state definitions */
#define PROCESS_RUNNING  0
#define PROCESS_KILLED   1
#define PROCESS_READY    2
#define PROCESS_NONE     3

struct cpu_regs {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t cpsr;
    uint32_t pc;
};


typedef struct {
    uint32_t used;
    uint32_t pid;
    uint32_t priority;
    uint32_t state;

    // Memory management
    uint32_t* ttbr0;// Physical address of translation table base
    uint32_t asid;        // Address Space ID (if using ASIDs) TODO

    uint32_t code_page_paddr;
    uint32_t data_page_paddr;
    uint32_t code_page_vaddr;
    uint32_t data_page_vaddr;
    uint32_t code_size;

    struct cpu_regs context;
    // rest of registers from user mode
} process_t;

process_t* create_process(void* code_page, void* data_page, uint8_t* bytes, size_t size);
process_t* clone_process(process_t* original_p);
void place_context_on_user_stack(struct cpu_regs* regs, uint32_t* stack_top);
void get_kernel_regs(struct cpu_regs* regs);
extern process_t* current_process;
int scheduler_init(void);
void scheduler(void);

// asm
extern void context_switch_1(struct cpu_regs* next_context);

#endif // KERNEL_SCHED_H
