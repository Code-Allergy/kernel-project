.section .vectors, "ax"
.global _vectors
.extern current_process
.align 5
_vectors:
    ldr pc, reset_addr    /* 0x00 Reset */
    ldr pc, undef_addr    /* 0x04 Undefined Instruction */
    ldr pc, swi_addr      /* 0x08 SWI */
    ldr pc, prefetch_addr /* 0x0C Prefetch Abort */
    ldr pc, data_addr     /* 0x10 Data Abort */
    nop                   /* 0x14 Reserved */
    ldr pc, irq_addr      /* 0x18 IRQ */
    ldr pc, fiq_addr      /* 0x1C FIQ */

/* Address table */
reset_addr:    .word reset_handler
undef_addr:    .word undef_handler
swi_addr:      .word swi_handler
prefetch_addr: .word prefetch_abort_handler
data_addr:     .word data_abort_handler
irq_addr:      .word irq_handler
fiq_addr:      .word fiq_handler

/* Default handlers */


.section .text
reset_handler:
fiq_handler:
    b .  /* Infinite loop for unhandled exceptions */

undef_handler:
    /* Save Undefined mode LR and SPSR */
    stmfd   sp!, {lr}
    mrs     lr, spsr
    stmfd   sp!, {lr}

    /* Switch to System mode to access user registers */
    cps     #0x1F                /* System mode */

    /* Store user registers in process struct */
    ldr     r1, =current_process
    ldr     r1, [r1]
    add     r1, r1, #20          /* process_t->regs offset (adjust if your struct differs) */

    stmia   r1!, {r0-r12}        /* Save R0-R12 */
    str     sp, [r1], #4         /* Save user SP */
    str     lr, [r1], #4         /* Save user LR */

    /* Restore exception context */
    cps     #0x1B                /* Undefined mode */
    ldmfd   sp!, {r2}            /* Get saved SPSR (user CPSR) */
    ldmfd   sp!, {r3}            /* Get exception LR (PC + 4) */
    sub     r3, r3, #4           /* Actual PC of faulting instruction */

    /* Store PC and CPSR in process struct */
    str     r3, [r1], #4         /* Save PC */
    str     r2, [r1], #4         /* Save CPSR */

    /* Switch to SVC mode for kernel processing */
    cps     #0x13                /* SVC mode */

    /* Read exception syndrome */
    mrc     p15, 0, r0, c5, c0, 0  /* Read ESR */

    /* Call C handler with ESR and process */
    ldr     r1, =current_process
    ldr     r1, [r1]
    bl      handle_undefined

    /* Schedule new process */
    b       scheduler

irq_handler:
    sub lr, lr, #4        /* Adjust LR for IRQ return */
    mrs r0, spsr          /* Save SPSR */
    push {r0, r1-r12, lr} /* Save context and SPSR */

    mov r0, lr            /* Pass return address as argument */
    bl handle_irq_c       /* Call C handler */

    pop {r0, r1-r12, lr}  /* Restore context and SPSR */
    msr spsr_cxsf, r0     /* Restore SPSR */
    subs pc, lr, #0       /* Return from exception */

swi_handler:
    STMFD   SP!, {R0-R3, LR}       @ Save R0-R3 and return address (LR)
    MOV     R0, R7                 @ Get SVC number from R7
    LDR     R1, [SP, #0]           @ Load original R0 (arg1) from stack
    LDR     R2, [SP, #4]           @ Load original R1 (arg2) from stack
    LDR     R3, [SP, #8]           @ Load original R2 (arg3) from stack
    LDR     R12, [SP, #12]         @ Load original R3 (arg4) from stack
    LDR     LR, [SP, #16]          @ Load return address (raddr) from stack
    STMFD   SP!, {R12, LR}         @ Push arg4 and raddr to stack (for C call)
    BL      handle_svc_c            @ Call C handler
    ADD     SP, SP, #8             @ Cleanup stack (arg4 and raddr)
    LDMFD   SP!, {R0-R3, PC}^      @ Restore registers and return

prefetch_abort_handler:
    b prefetch_abort_c

old_swi:
        @ Save registers
    STMFD   SP!, {R0-R3, LR}  @ Push R0-R3 and return address

    @ Read the SVC instruction from LR - 4
    SUB     LR, LR, #4
    LDR     R0, [LR]           @ Load SVC instruction
    BIC     R0, R0, #0xFF000000 @ Extract syscall number (last byte)
