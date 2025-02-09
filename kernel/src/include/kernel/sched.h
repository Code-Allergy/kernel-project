#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <stddef.h>
#include <stdint.h>



/* Process state definitions */
#define PROCESS_RUNNING  1
#define PROCESS_KILLED   2
#define PROCESS_READY    3
#define PROCESS_NONE     0

struct cpu_regs {
    uint32_t r4; // 0
    uint32_t r5; // 4
    uint32_t r6; // 8
    uint32_t r7; // 12
    uint32_t r8; // 16
    uint32_t r9; // 20
    uint32_t r10; // 24
    uint32_t r11; // 28
    uint32_t r12; // 32
    uint32_t sp; // 36
    uint32_t lr; // 40
    uint32_t cpsr; // 44
    uint32_t pc; // 48
};


typedef struct {
    uint32_t pid;
    uint32_t priority;
    uint32_t state;

    // Memory management
    uint32_t* ttbr0;      // Physical address of translation table base
    uint32_t asid;        // Address Space ID
    uint32_t kernel_cpsr;

    uint32_t code_size;
    uint32_t code_entry;
    uint32_t code_page_paddr;
    uint32_t code_page_vaddr;
    uint32_t data_page_paddr;
    uint32_t data_page_vaddr;
    uint32_t stack_page_paddr;
    uint32_t stack_page_vaddr;
    uint32_t stack_top;
    uint32_t heap_page_paddr;
    uint32_t heap_page_vaddr;

    struct cpu_regs context;
    // rest of registers from user mode
} process_t;

process_t* create_process(uint8_t* bytes, size_t size);
process_t* clone_process(process_t* original_p);
void place_context_on_user_stack(struct cpu_regs* regs, uint32_t* stack_top);
void get_kernel_regs(struct cpu_regs* regs);
extern process_t* current_process;
int scheduler_init(void);
void scheduler(void);

// asm
extern void context_switch(struct cpu_regs* old_context, struct cpu_regs* new_context);
extern void context_switch_1(struct cpu_regs* next_context);

#endif // KERNEL_SCHED_H
