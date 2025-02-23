.text
.align 4

@ r0 is the stack pointer of the current context, with all registers saved
.global user_context_return
.type user_context_return, %function
user_context_return:
    ldr r1, [r0, #56]           @ Load CSPR from the saved stack
    ldr r2, [r0, #60]           @ Load PC from the saved stack
    push {r1, r2}               @ Push CSPR and PC onto the stack

    cps    #0x1F                @ Switch to System mode

    mov   sp, r0               @ Restore user mode sp, then load r0-r12 and LR
    ldmia sp, {r0-r12, lr}
    add sp, sp, #64            @ advance the stack pointer past the saved registers

    cps    #0x13               @ Switch to Supervisor mode again (needed before returning)

    pop   {lr}                 @ Load saved CSPR from the stack into lr
    msr   spsr_cxsf, lr

    pop   {lr}                 @ Load saved PC from the stack
    movs  pc, lr
