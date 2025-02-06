#include <stdint.h>
#include <kernel/printk.h>
#include <kernel/int.h>
#include <kernel/uart.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/intc.h>

#include "intc.h"
#include "int.h"
#include "../../../team-repo/src/drivers/qemu/uart.h"

// handle interrupts here
uint32_t svc_handlers[NR_SYSCALLS] = {0};

void handle_svc_c(void) {
    // get syscall number
    uint32_t svc_number;

    asm volatile("mov %0, r7" : "=r"(svc_number));

    if (svc_number == 0) {
        char* user_char_ptr;
        uint32_t user_char_len;
        asm volatile("mov %0, r0" : "=r"(user_char_ptr));
        asm volatile("mov %0, r1" : "=r"(user_char_len));
        printk("U: %s", user_char_ptr);
        return;
    } else if (svc_number == 1) {
        uint32_t sleep_time;
        asm volatile("mov %0, r0" : "=r"(sleep_time));
        printk("Sleeping for %d\n", sleep_time);
        // start timer1 for sleep_time
        return;
    }


    printk("SVC handler: %d\n", svc_number);


    while(1);
}

void data_abort_handler(uint32_t lr) {
    uint32_t dfsr, dfar, status, domain;

    // Read the Fault Status Register (DFSR)
    __asm__ volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr));
    // Read the Fault Address Register (DFAR)
    __asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar));

    // Extract status bits (bits [4:0] and bit [10])
    status = (dfsr & 0b1111) | ((dfsr >> 6) & 0b10000);
    domain = (dfsr >> 4) & 0xF;  // Fault domain

    printk("Data Abort at address: %p\n", dfar);
    printk("Fault Status: %p (Domain: %d)\n", status, domain);

    switch (status) {
        case 0b00001: printk("Alignment fault\n"); break;
        case 0b00011: printk("Debug event\n"); break;
        case 0b00100: printk("Instruction cache maintenance fault\n"); break;
        case 0b01100: printk("Translation fault (Level 1)\n"); break;
        case 0b01110: printk("Translation fault (Level 2)\n"); break;
        case 0b01000: printk("Access flag fault (Level 1)\n"); break;
        case 0b01010: printk("Access flag fault (Level 2)\n"); break;
        case 0b00110: printk("Domain fault (Level 1)\n"); break;
        case 0b00111: printk("Domain fault (Level 2)\n"); break;
        case 0b01101: printk("Permission fault (Level 1)\n"); break;
        case 0b01111: printk("Permission fault (Level 2)\n"); break;
        case 0b01001: printk("Synchronous External Abort\n"); break;
        case 0b01011: printk("TLB conflict abort\n"); break;
        case 0b10100: printk("Lockdown fault\n"); break;
        case 0b10110: printk("Coprocessor fault\n"); break;
        case 0b00000: printk("Unknown fault\n"); break;
        default: printk("Reserved fault status\n"); break;
    }

    printk("Returning to LR: %p\n", lr);

    // Kernel panic or recovery logic
    while (1);
}

void prefetch_abort_c() {
    uint32_t ifar, ifsr;
    asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r"(ifar)); // IFAR
    asm volatile("mrc p15, 0, %0, c5, c0, 2" : "=r"(ifsr)); // IFSR
    printk("Prefetch abort! Addr: %p, Status: %p\n", ifar, ifsr);
    while (1); // Halt
}

void uart_handler(int irq, void *data) {
    // Read UART IIR to check interrupt type and clear it
    uint32_t iir = UART0->IIR_FCR & 0x0F;  // Mask IIR bits

    if (iir == 0x04) {  // Received data available
        char c = UART0->RBR_THR_DLL;  // Read the character (clears interrupt)
        printk("Received: %c\n", c);
    } else if (iir == 0x0C) {  // Timeout (FIFO non-empty)
        // Handle FIFO timeout
    }

    // Acknowledge interrupt in INTC (platform-specific)
    INTC->IRQ_PEND[0] = (irq << 1);

    printk("IRQ %d: UART fired!\n", irq);
}

void handle_undefined(uint32_t esr, process_t* p) {
    const uint32_t ec = esr >> 26;
    const uint32_t il = (esr >> 25) & 1;
    const uint32_t iss = esr & 0x1FFFFFF;

    // Log exception details
    printk("Undefined Instruction in PID %d at %p\n",
           p->pid, p->regs.pc);
    printk("Instruction: %p\n", *(uint32_t*)p->regs.pc);
    printk("ESR: %p (EC=%p IL=%d ISS=%p)\n",
           esr, ec, il, iss);

    // Mark process as killed
    p->state = PROCESS_KILLED;

    // Optional: Dump register state
    #ifdef DEBUG
    dump_registers(&p->regs);
    #endif
}
