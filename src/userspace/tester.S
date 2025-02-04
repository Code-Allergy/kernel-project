    .syntax unified       /* Use unified ARM/Thumb syntax */
    .global _start        /* Declare the entry point */

.global syscall_test
_start:
    mov r7, #60       @ Syscall number for 'write' (e.g., Linux syscall 4)
    svc #0           @ Trigger syscall