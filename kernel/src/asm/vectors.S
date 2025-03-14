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
swi_addr:      .word swi_stack_handler
prefetch_addr: .word prefetch_abort_handler
data_addr:     .word data_abort_handler
irq_addr:      .word irq_handler_new
fiq_addr:      .word fiq_handler

/* Default handlers */

.section .text
reset_handler:
fiq_handler: /* No FIQ hardware on AM335x */
    b .  /* Infinite loop for unhandled exceptions */

prefetch_abort_handler:
    b prefetch_abort_c

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

@ TODO: cleanup, don't clobber r12, do 2 steps to save lr
irq_handler_new:
    cpsid    I                           @ disable all interrupts
    MRS      R12, SPSR                   @ Get SPSR
    @ Check if we were in user mode before the interrupt
    AND      R12, R12, #0x1F             @ Mask out everything except mode bits
    CMP      R12, #0x10                  @ Compare with user mode (0x10)
    BNE      irq_from_kernel             @ Branch if not user mode
    MRS      R12, SPSR                   @ Reset SPSR in R12

    @ ----------------- User mode process -----------------
    CPS   #0x1F                        @ System mode
    SUB  SP, SP, #64                  @ Allocate space for R0-R12, LR, PC and SPSR

    @ store user registers
    STMIA   SP, {R0-R12, LR}           @ Save user registers
    STR     R12, [SP, #56]              @ Save SPSR at offset 60
    MOV     R0, SP                      @ Get user SP
    CPS    #0x12                       @ Switch to IRQ mode
    MOV   R12, LR
    SUB   R12, R12, #4
    STR   R12, [R0, #60]              @ Save user PC (LR contains return address)

    @ switch to supervisor mode to handle the interrupt
    CPS     #0x13                       @ Switch to supervisor mode to do the actual interrupt

    BL      handle_irq_c               @ Call C handler
    B      userspace_return

@ handle interrupt in kernel space
irq_from_kernel:
    STMDB   SP!, {R0-R12, LR}             @ Save kernel registers
    MRS     R0, SPSR                      @ Get SPSR_irq
    STMDB   SP!, {R0}                     @ Save SPSR onto the stack


    MOV    R0, #0                         @ load 0 to R0 to indicate kernel mode
    BL      handle_irq_c                  @ Call IRQ handler

    @ Restore registers
    LDMIA   SP!, {R0}                     @ Restore SPSR from stack
    MSR     SPSR_cxsf, R0                 @ Restore SPSR
    LDMIA   SP!, {R0-R12, LR}              @ Restore registers

    SUBS    PC, LR, #4                     @ Return to kernel (adjust IRQ offset)



swi_stack_handler:
    @ Save critical registers and LR/SPSR to Supervisor stack
    STMFD   SP!, {R0, LR}      @ Save R0 and Supervisor LR
    MRS     R0, SPSR              @ Get user's CPSR from SPSR
    STMFD   SP!, {R0}             @ Save SPSR to Supervisor stack

    @ Switch to System mode (user registers now accessible)
    CPS     #0x1F
    SUB     SP, SP, #64           @ 16 registers * 4 = 64 bytes

    @ Save user's R0-R12 and LR (System mode's registers)
    STMIA   SP, {R0-R12, LR}      @ Safe: uses user's actual registers
    MOV     R0, SP                @ R0 = user's SP

    @ Switch back to Supervisor mode to retrieve saved LR/SPSR
    CPS     #0x13
    LDMFD   SP!, {R1}             @ R1 = user's CPSR (from SPSR)
    LDMFD   SP!, {R2, LR}         @ R2 = user's R0, LR = user's LR

    @ Store R0 and PC(lr) and CPSR to user stack
    STR     R2, [R0, #0]         @ Save R0 at offset 0
    STR     R1, [R0, #56]         @ Save CPSR at offset 56
    STR     LR, [R0, #60]         @ Save PC at offset 60

    @ Pass arguments to handle_svc_c and switch back to Supervisor mode for the call
    MOV     R1, R0                @ User stack pointer (context)
    MOV     R0, R7                @ Syscall number (from R7)

    BL      handle_svc_c
    B       userspace_return
