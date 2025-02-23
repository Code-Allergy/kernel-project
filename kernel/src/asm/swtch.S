.global context_switch
.global context_restore

.text
.align 4

@ TODO this shouldn't clobber any of r0-r12
@ r0 is the stack pointer of the current context, with all registers saved
.global user_context_return
.type user_context_return, %function
user_context_return:
    cpsid i                    @ Disable interrupts
    cps    #0x1F                @ Switch to System mode

    @ Restore user mode sp, then load r0-r12
    mov   sp, r0               @ Restore user SP

    @ set spsr to the saved cpsr
    ldmia sp, {r0-r12, lr}     @ Load r0-r12, LR
    ldr   ip, [sp, #60]        @ Load saved PC into temporary register (ip/r12)
    ldr   r10, [sp, #56]        @ Load saved CPSR

    @ advance the stack pointer past the last 4
    add sp, sp, #64            @ update stack

    cps    #0x13               @ Switch to Supervisor mode again (needed before returning)
    @ Set SPSR to the saved CPSR (user's CPSR)
    msr   spsr_cxsf, r10

    @ Set LR to the saved PC (adjusted for exception return)
    mov   lr, ip


    cpsie if                  @ Re-enable interrupts in supervisor mode
    @ Return to user mode, restoring CPSR from SPSR
    movs  pc, lr
