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

irq_handler:
    sub lr, lr, #4        /* Adjust LR for IRQ return */
    mrs r0, spsr          /* Save SPSR */
    /* push {r0, r1-r12, lr} Save context and SPSR */

    mov r0, lr            /* Pass return address as argument */
    b handle_irq_c       /* Call C handler */

    /* pop {r0, r1-r12, lr}   Restore context and SPSR */
    /* msr spsr_cxsf, r0     Restore SPSR */
    /* subs pc, lr, #0        Return from exception */

@ TODO: cleanup, don't clobber r12, do 2 steps to save lr
irq_handler_new:
    MOV   R12, LR
    CPS   #0x1F                        @ System mode
    SUB  SP, SP, #64                  @ Allocate space for R0-R12, LR, PC and SPSR

    MOV   LR, R12                      @ store user registers
    @ store user registers
    STMIA   SP, {R0-R12, LR}           @ Save user registers
    STR     LR, [SP, #56]              @ Save user PC (LR contains return address)

    MRS     R0, SPSR                    @ Get SPSR
    STR     R0, [SP, #60]              @ Save SPSR at offset 60

    @ save stack pointer for passing to C handler
    MOV     R0, SP                      @ Get system mode SP

    @ switch back to IRQ mode
    CPS     #0x12                       @ Switch to IRQ mode

    BL      handle_irq_c               @ Call C handler
    B      userspace_return

swi_stack_handler:
    @ save 0-3 on stack
    STMFD   SP!, {R0-R3}
    MOV     R0, LR

    @ switch to system mode, load user registers into process stack
    CPS    #0x1F                        @ System mode

    @ allocate stack space for user registers
    SUB    SP, SP, #64                  @ Allocate space for R0-R12, LR, PC and SPSR

    STMIA   SP, {R0-R12, LR}       @ Save R0-R3 and return address (LR)
    @ move return address to PC
    STR     R0, [SP, #60]              @ Save user PC (LR contains return address)

    MRS     R0, SPSR                    @ Get SPSR
    STR     R0, [SP, #56]               @ Save SPSR
    MOV     R0, SP                      @ Get SP

    CPS    #0x13                       @ Switch to supervisor mode
    LDMFD   SP!, {R1-R4}                   @ Restore old R0-R2, leave R3 on stack
        @ Stack additional parameters (right to left)
    STMFD   SP!, {R0}              @ Push user SP (6th param)
    STMFD   SP!, {R4}              @ Push original R3 (5th param)
    MOV     R0, R7                @ Get SVC number from R7


    BL     handle_svc_c            @ Call C handler
    ADD     SP, SP, #8             @ Remove 5th and 6th params from stack
    B      userspace_return
