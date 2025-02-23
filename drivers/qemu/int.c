#include <stdint.h>
#include <kernel/printk.h>
#include <kernel/int.h>
#include <kernel/uart.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/intc.h>
#include <kernel/log.h>

#include <kernel/boot.h>
#include <kernel/mmu.h>
#include <kernel/errno.h>
#include <kernel/panic.h>

// handle interrupts here
uint32_t svc_handlers[NR_SYSCALLS] = {0};

void handle_svc_c(
    uint32_t svc_number,  // SVC number (extracted from instruction)
    uint32_t* sp) {
    current_process->stack_top = sp;
    LOG_SYSCALL(svc_number);
    if (svc_number > NR_SYSCALLS) {
        sp[0] = -ENOSYS;
        return;
    }

    handle_syscall(svc_number, sp[0], sp[1], sp[2], sp[3]);
}

void data_abort_handler(uint32_t lr) {
    uint32_t dfsr, dfar, status, domain;

    // Read the Fault Status Register (DFSR)
    __asm__ volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr));
    // Read the Fault Address Register (DFAR)
    __asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar));

    // Extract status bits (bits [4:0] and bit [10])
    status = (dfsr & 0xF) | ((dfsr >> 6) & 0x10);
    domain = (dfsr >> 4) & 0xF;  // Fault domain

    // switch page table so we can print
    uint32_t kernel_l1_phys = ((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE;
    mmu_driver.set_l1_table((uint32_t*) kernel_l1_phys);

    printk("Data Abort at address: %p\n", dfar);
    printk("Fault Status: %p (Domain: %d)\n", status, domain);

    switch (status) {
        case 0x01: printk("Alignment fault\n"); break;
        case 0x03: printk("Debug event\n"); break;
        case 0x04: printk("Instruction cache maintenance fault\n"); break;
        case 0x0C: printk("Translation fault (Level 1)\n"); break;
        case 0x0E: printk("Translation fault (Level 2)\n"); break;
        case 0x08: printk("Access flag fault (Level 1)\n"); break;
        case 0x0A: printk("Access flag fault (Level 2)\n"); break;
        case 0x06: printk("Domain fault (Level 1)\n"); break;
        case 0x07: printk("Domain fault (Level 2)\n"); break;
        case 0x0D: printk("Permission fault (Level 1)\n"); break;
        case 0x0F: printk("Permission fault (Level 2)\n"); break;
        case 0x09: printk("Synchronous External Abort\n"); break;
        case 0x0B: printk("TLB conflict abort\n"); break;
        case 0x14: printk("Lockdown fault\n"); break;
        case 0x16: printk("Coprocessor fault\n"); break;
        case 0x00: printk("Unknown fault\n"); break;
        default: printk("Reserved fault status\n"); break;
    }

    printk("Returning to LR: %p\n", lr);

    // Kernel panic or recovery logic
    panic("Stopping");
}



void prefetch_abort_c(void) {
    uint32_t ifar, ifsr;
    __asm__ volatile("mrc p15, 0, %0, c6, c0, 2" : "=r"(ifar)); // IFAR
    __asm__ volatile("mrc p15, 0, %0, c5, c0, 2" : "=r"(ifsr)); // IFSR
    panic("Prefetch abort! Addr: %p, Status: %p\n", ifar, ifsr);
}

void handle_undefined(uint32_t esr, process_t* p) {
    const uint32_t ec = esr >> 26;
    const uint32_t il = (esr >> 25) & 1;
    const uint32_t iss = esr & 0x1FFFFFF;

    // Log exception details
    printk("Undefined Instruction in PID %d at TODO\n",
           p->pid);
    // printk("Instruction: %p\n", *(uint32_t*)p->context.pc);
    printk("ESR: %p (EC=%p IL=%d ISS=%p)\n",
           esr, ec, il, iss);

    // Mark process as killed
    p->state = PROCESS_KILLED;

    // Optional: Dump register state
    // #ifdef DEBUG
    // dump_registers(&p->regs);
    // #endif
}
