.global context_switch
.global context_restore

.type context_switch, %function

.global context_switch
.text
.align 4

/* void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context) */
context_switch:
    /* Save current context */
    str     r0, [r0, #0]         @ Save r0
    str     r1, [r0, #4]         @ Save r1
    str     r2, [r0, #8]         @ Save r2
    str     r3, [r0, #12]        @ Save r3
    str     r4, [r0, #16]        @ Save r4
    str     r5, [r0, #20]        @ Save r5
    str     r6, [r0, #24]        @ Save r6
    str     r7, [r0, #28]        @ Save r7
    str     r8, [r0, #32]        @ Save r8
    str     r9, [r0, #36]        @ Save r9
    str     r10, [r0, #40]      @ Save r10
    str     r11, [r0, #44]      @ Save r11
    str     r12, [r0, #48]      @ Save r12
    str     sp, [r0, #52]       @ Save sp
    str     lr, [r0, #56]       @ Save lr
    mrs     r2, cpsr
    str     r2, [r0, #60]       @ Save cpsr
    str     lr, [r0, #64]       @ Save pc (current lr is next pc)

    /* Load new context from next_context (r1) */
    ldr     r2, [r1, #60]       @ Load saved cpsr
    msr     spsr_cxsf, r2       @ Set saved program status register

    /* Load all registers from new context */
    ldr     sp, [r1, #52]       @ Restore sp
    ldr     lr, [r1, #56]       @ Restore lr
    ldr     r0, [r1, #0]        @ Restore r0
    ldr     r2, [r1, #8]        @ Restore r2
    ldr     r3, [r1, #12]       @ Restore r3
    ldr     r4, [r1, #16]       @ Restore r4
    ldr     r5, [r1, #20]       @ Restore r5
    ldr     r6, [r1, #24]       @ Restore r6
    ldr     r7, [r1, #28]       @ Restore r7
    ldr     r8, [r1, #32]       @ Restore r8
    ldr     r9, [r1, #36]       @ Restore r9
    ldr     r10, [r1, #40]      @ Restore r10
    ldr     r11, [r1, #44]      @ Restore r11
    ldr     r12, [r1, #48]      @ Restore r12

    /* Restore r1 last since we're using it for next_context */
    ldr     r1, [r1, #4]        @ Restore r1

    /* Switch to user mode and restore pc */
    ldr     pc, [r1, #64]       @ Load saved pc and switch

.global context_switch_1
/* void context_switch(struct cpu_regs* next_context) */
context_switch_1:
    /* First, load CPSR to prepare for mode switch */
    ldr     r1, [r0, #60]       @ Load saved cpsr from context
    msr     spsr_cxsf, r1       @ Set up spsr for mode switch

    /* Load user registers */
    ldm     r0, {r4-r12}        @ Restore r4-r12
    ldr     r1, [r0, #52]       @ Get stored SP
    mov     sp, r1              @ Restore SP
    ldr     lr, [r0, #56]       @ Restore LR

    /* Load arguments into r0-r3 last since they're caller saved */


    /* Load PC and switch modes atomically */
    ldr     r1, [r0, #64]       @ Load PC value
    movs    pc, r1              @ Switch to user mode and start execution

.global context_restore
.global context_restore
context_restore:
    ldmia sp!, {r0-r12}        // Restore registers from stack
    ldr lr, [sp, #4]            // Restore link register
    ldr sp, [sp]                // Restore stack pointer
    movs pc, lr                 // Return to caller
