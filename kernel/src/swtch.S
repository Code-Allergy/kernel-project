.global context_switch
.global context_restore

.text
.align 4

/* void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context) */
.global context_switch
context_switch:
    /* First save the current context to old_context (r0) */
    push    {r4-r12, lr}         @ Save SVC mode registers temporarily

    /* Save callee-saved registers to old context */
    stm     r0, {r4-r12}         @ Store r4-r12 to old context

    /* Switch to System mode to access user sp/lr */
    mrs     r2, cpsr             @ Save current mode
    cps     #0x1F                @ Switch to System mode

    /* Save user mode sp/lr to old context */
    str     sp, [r0, #36]        @ Save user SP
    str     lr, [r0, #40]        @ Save user LR

    /* Load new user mode sp/lr from new context */
    ldr     sp, [r1, #36]        @ Restore new SP
    ldr     lr, [r1, #40]        @ Restore new LR

    /* Switch back to SVC mode */
    cps     #0x13                @ Return to SVC mode

    /* Load CPSR for final switch */
    ldr     r2, [r1, #44]        @ Load new saved cpsr
    msr     spsr_cxsf, r2        @ Set up spsr

    /* Restore registers from new context */
    ldm     r1, {r4-r12}         @ Restore r4-r12

    /* Perform mode switch and jump */
    ldr     pc, [r1, #48]        @ Load and switch to new PC

.global context_switch_1
context_switch_1:
    push    {r4-r12, lr}         @ Save SVC mode registers

    /* Load CPSR for final switch */
    ldr     r1, [r0, #44]        @ Load saved cpsr
    msr     spsr_cxsf, r1        @ Set up spsr

    /* Switch to System mode */
    cps     #0x1F                @ Switch to System mode directly

    /* Set up User mode registers */
    ldr     r1, [r0, #36]        @ Get stored SP
    mov     sp, r1               @ Set up User SP
    ldr     r1, [r0, #40]        @ Get stored LR
    mov     lr, r1               @ Set up User LR

    /* Return to SVC mode */
    cps     #0x13                @ Switch back to SVC mode

    /* Restore registers and switch */
    ldm     r0, {r4-r12}         @ Restore r4-r12
    ldr     r1, [r0, #48]        @ Load PC value
    movs    pc, r1               @ Switch to user mode and start execution

.global context_restore
.global context_restore
context_restore:
    ldmia sp!, {r0-r12}        // Restore registers from stack
    ldr lr, [sp, #4]            // Restore link register
    ldr sp, [sp]                // Restore stack pointer
    movs pc, lr                 // Return to caller
