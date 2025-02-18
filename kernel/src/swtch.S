.global context_switch
.global context_restore

.text
.align 4

/* void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context) */
.global context_switch
context_switch:
    /* First save the current context to old_context (r0) */
    @push    {r4-r12, lr}         @ Save SVC mode registers temporarily

    /* Save callee-saved registers to old context */
    add     r0, r0, #16          @ Adjust base address to start at r4
    add     r1, r1, #16          @ Adjust base address to start at r4
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
    @push    {r4-r12, lr}          Save SVC mode registers

    /* Load CPSR for final switch */
    ldr     r1, [r0, #60]        @ Load saved cpsr
    msr     spsr_cxsf, r1        @ Set up spsr

    /* Switch to System mode */
    cps     #0x1F                @ Switch to System mode directly

    /* Set up User mode registers */
    ldr     r1, [r0, #52]        @ Get stored SP
    mov     sp, r1               @ Set up User SP
    ldr     r1, [r0, #56]        @ Get stored LR
    mov     lr, r1               @ Set up User LR

    /* Return to SVC mode */
    cps     #0x13                @ Switch back to SVC mode

    /* Restore registers and switch */
    add     r0, r0, #16          @ Adjust base address to start at r4
    ldm     r0, {r4-r12}         @ Restore r4-r12
    ldr     r1, [r0, #48]        @ Load PC value
    movs    pc, r1               @ Switch to user mode and start execution

.global syscall_return
syscall_return:
    /* r0 contains struct pointer */
    /* r1 contains return value to set */

    /* Store return value in r0 position */
    str     r1, [r0, #0]         @ Store return value in r0 slot

    /* Restore r1-r3 */
    ldr     r1, [r0, #4]         @ Restore r1
    ldr     r2, [r0, #8]         @ Restore r2
    ldr     r3, [r0, #12]        @ Restore r3

    /* Restore r4-r12 */
    add     r0, r0, #16          @ Point to r4
    ldm     r0, {r4-r12}         @ Restore r4-r12

    /* Switch to System mode to restore sp and lr */
    cps     #0x1F                @ Switch to System mode

    /* Restore user sp and lr */
    ldr     r0, [r0, #36]        @ Load SP (offset from r4 base)
    mov     sp, r0               @ Restore SP
    ldr     r0, [r0, #40]        @ Load LR (offset from r4 base)
    mov     lr, r0               @ Restore LR

    /* Switch back to SVC mode */
    cps     #0x13

    /* Restore r0 (return value) */
    ldr     r0, [r0, #-16]       @ Load return value from r0 slot

    /* Return to user mode */
    movs    pc, lr               @ Return and switch to user mode


@ r0 is the stack pointer of the current context, with all registers saved
.global user_context_return
user_context_return:
    cps    #0x1F                @ Switch to System mode

    @ Restore user mode sp, then load r0-r12
    mov   sp, r0               @ Restore user SP

    @ set spsr to the saved cpsr
    ldr  r0, [sp, #60]        @ Load saved cpsr
    msr  spsr_cxsf, r0        @ Set spsr register for mode switch

    ldmia sp, {r0-r12, lr}         @ Restore r0-r12

    @ advance the stack pointer past the last 4
    add sp, sp, #64         @ Skip over cpsr

    movs pc, lr              @ Jump to return address
