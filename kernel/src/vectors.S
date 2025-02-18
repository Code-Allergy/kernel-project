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

_swi_stack_handler:
    @ save 0-3 on svc stack
    STMFD   SP!, {R0-R3}
    MOV     R0, LR
    mrs r1, spsr   @ This should give you the User mode CPSR (0x10)
    and r1, r1, #0x1F

    @ switch to system mode, load user registers into process stack
    CPS    #0x1F                        @ System mode
    @ allocate stack space for user registers
    SUB    SP, SP, #64                  @ Allocate space for R0-R12, LR, PC and SPSR

    STMIA   SP, {R0-R12, LR}       @ Save R0-R3 and return address (LR)
    STR     R0, [SP, #60]         @ Save PC at offset 60 (16th slot)
    STR     R1, [SP, #56]         @ Save CPSR at offset 56 (15th slot)

    @ Prepare arguments for handle_svc_c
    MOV     R0, SP                @ User stack pointer = context pointer
    CPS     #0x13                 @ Switch back to Supervisor mode

    @ Restore original r0-r3 from supervisor stack
    LDMFD   SP!, {R1-R4}          @ R1-R4 now hold original r0-r3

    @ Push arguments for handle_svc_c (syscall_num, context_ptr)
    STMFD   SP!, {R0}             @ Push context pointer (1st stack arg)
    STMFD   SP!, {R4}             @ Push r0 (2st stack arg)
    MOV     R0, R7                @ Syscall number from r7 (0st arg)

    BL     handle_svc_c            @ Call C handler
    ADD   SP, SP, #4              @ Pop context pointer
    B      userspace_return

swi_stack_handler:
    @ Save critical registers and LR/SPSR to Supervisor stack
    STMFD   SP!, {R0, LR}      @ Save R0-R3 and Supervisor LR
    MRS     R0, SPSR              @ Get user's CPSR from SPSR
    STMFD   SP!, {R0}             @ Save SPSR to Supervisor stack

    @ Switch to System mode (user registers now accessible)
    CPS     #0x1F

    @ Allocate space on user stack for full context
    SUB     SP, SP, #64           @ 16 registers * 4 = 64 bytes

    @ Save user's R0-R12 and LR (System mode's registers)
    STMIA   SP, {R0-R12, LR}      @ Safe: uses user's actual registers

    @ Switch back to Supervisor mode to retrieve saved LR/SPSR
    CPS     #0x13
    LDMFD   SP!, {R0}             @ R0 = user's CPSR (from SPSR)
    LDMFD   SP!, {R1, LR}      @ Restore R0-R3, LR (SWI return address)
    MOV     R5, LR                @ Save LR for later

    @ Switch to System mode to save PC/CPSR to user stack
    CPS     #0x1F
    @ Store R0-R3 (which are in R1-R4) to user stack
    STR     R1, [SP, #0]         @ Save R0 at offset 0
@    STR     R2, [SP, #4]         @ Save R1 at offset 4
@    STR     R3, [SP, #8]         @ Save R2 at offset 8
@    STR     R4, [SP, #12]        @ Save R3 at offset 12

    STR     R5, [SP, #60]         @ Save PC at offset 60
    STR     R0, [SP, #56]         @ Save CPSR at offset 56

    @ Pass arguments to handle_svc_c and switch back to Supervisor mode for the call
    MOV     R1, SP                @ User stack pointer (context)
    MOV     R0, R7                @ Syscall number (from R7)
    CPS     #0x13                 @ Back to Supervisor mode

    BL      handle_svc_c
    B       userspace_return
