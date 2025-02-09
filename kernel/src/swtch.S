.global context_switch
.global context_restore

.text
.align 4

/* void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context) */
.global context_switch
context_switch:
    @ r0 = current context, r1 = next context
    @ First save the current context
    str     lr, [r0, #64]       @ Save current PC (in LR) first
    stm     r0, {r4-r12}        @ Save r4-r12
    str     sp, [r0, #52]       @ Save SP
    str     lr, [r0, #56]       @ Save LR
    mrs     r2, cpsr            @ Get current cpsr
    str     r2, [r0, #60]       @ Save CPSR

    @ Now ready to load next context (similar to context_switch_1)
    mov     r0, r1              @ Move next context pointer to r0
    ldr     r1, [r0, #60]       @ Load saved cpsr
    msr     spsr_cxsf, r1       @ Set up spsr for mode switch
    ldm     r0, {r4-r12}        @ Restore r4-r12
    ldr     r1, [r0, #52]       @ Get stored SP
    mov     sp, r1              @ Restore SP
    ldr     lr, [r0, #56]       @ Restore LR
    ldr     r1, [r0, #64]       @ Load PC value
    movs    pc, r1              @ Switch to user mode and start execution

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
