.global context_switch
.global context_restore

.type context_switch, %function

context_switch:
    // Arguments:
    // r0 = old process (current process)
    // r1 = new process (next process)

    // Save Current Process Registers
    mrs r2, cpsr                 // Get CPSR (current mode)
    stmdb r0!, {r4-r12, sp, lr}   // Store registers into old process struct
    str r2, [r0, #60]            // Save CPSR
    str lr, [r0, #64]            // Save return address (LR)
    str r15, [r0, #68]           // Save PC

    // Load New Process Registers
    ldmia r1!, {r4-r12, sp, lr}   // Load registers from new process struct
    ldr r2, [r1, #60]             // Load saved CPSR (only mode bits)

    // Transition to User Mode via SVC
    msr spsr, r2                  // Load SPSR (will apply on exception return)
    ldr r3, [r1, #68]             // Load PC (entry point of process)
    
    movs lr, r3                   // Set LR to process entry
    svc #0                        // Supervisor call to drop to User mode

.global context_restore
context_restore:
    /* r0 = &process->regs */
    ldmia   r0, {r1-r12, sp, lr}   /* Load R1-R12, SP, LR */
    ldr     ip, [r0, #60]          /* Load PC from regs.pc */
    ldr     r0, [r0, #64]          /* Load CPSR from regs.cpsr */
    
    msr     spsr_cxsf, r0          /* Set SPSR for mode return */
    mov     r0, #0                 /* Clear r0 for user code */
    
    /* Return to user mode */
    movs    pc, ip                 /* Atomic PC/CPSR update */
.size context_switch, . - context_switch

