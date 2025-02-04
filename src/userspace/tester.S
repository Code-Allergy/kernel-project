    .syntax unified       /* Use unified ARM/Thumb syntax */
    .global _start        /* Declare the entry point */
_start:
    ldr r0, =test_output         // r0 = address of test_output
    mov r1, #22                  // r1 = length of the string
    
    mov r7, #0               // Syscall number for debug
    svc #0                   // Trigger syscall


test_output:
    .asciz "Hello from userspace!"