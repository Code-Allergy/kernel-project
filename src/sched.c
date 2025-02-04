#include <stdint.h>
#include <kernel/sched.h>
#include <kernel/paging.h>

#define MAX_PROCESSES 16

process_t* current_process = NULL;
process_t process_table[MAX_PROCESSES];

// never return
int scheduler_init() {
    while(1) {
        asm volatile("wfi");
    }
}

void schedule(void) {
    // Your scheduling logic here
    // For example: Round Robin implementation
    static int last_pid = 0;
    
    do {
        last_pid = (last_pid + 1) % MAX_PROCESSES;
    } while(process_table[last_pid].state != PROCESS_READY);
    
    current_process = &process_table[last_pid];
    
    // Perform actual context switch (you'll need assembly for this)
    context_switch(&current_process->regs);
}


process_t* create_process(uint32_t code_page, uint32_t data_page) {
    process_t* proc = &process_table[0];

    proc->pid = 0;
    proc->priority = 0;
    proc->state = 0;

    proc->code_page = code_page;
    proc->data_page = data_page;

    proc->regs.r0 = 0;
    proc->regs.r1 = 0;
    proc->regs.r2 = 0;
    proc->regs.r3 = 0;
    proc->regs.r4 = 0;
    proc->regs.r5 = 0;
    proc->regs.r6 = 0;
    proc->regs.r7 = 0;
    proc->regs.r8 = 0;
    proc->regs.r9 = 0;
    proc->regs.r10 = 0;
    proc->regs.r11 = 0;
    proc->regs.r12 = 0;
    proc->regs.sp = data_page + PAGE_SIZE;
    proc->regs.pc = code_page;
    proc->regs.cpsr = 0x10; // user mode

    // TODO
    proc->regs.lr = 0;

    return proc;
}

void get_kernel_regs(struct cpu_regs* regs) {
    asm volatile("mrs %0, cpsr" : "=r"(regs->cpsr));
    asm volatile("mov %0, r0" : "=r"(regs->r0));
    asm volatile("mov %0, r1" : "=r"(regs->r1));
    asm volatile("mov %0, r2" : "=r"(regs->r2));
    asm volatile("mov %0, r3" : "=r"(regs->r3));
    asm volatile("mov %0, r4" : "=r"(regs->r4));
    asm volatile("mov %0, r5" : "=r"(regs->r5));
    asm volatile("mov %0, r6" : "=r"(regs->r6));
    asm volatile("mov %0, r7" : "=r"(regs->r7));
    asm volatile("mov %0, r8" : "=r"(regs->r8));
    asm volatile("mov %0, r9" : "=r"(regs->r9));
    asm volatile("mov %0, r10" : "=r"(regs->r10));
    asm volatile("mov %0, r11" : "=r"(regs->r11));
    asm volatile("mov %0, r12" : "=r"(regs->r12));
    asm volatile("mov %0, sp" : "=r"(regs->sp));
    asm volatile("mov %0, lr" : "=r"(regs->lr));
    asm volatile("mov %0, pc" : "=r"(regs->pc));
}
