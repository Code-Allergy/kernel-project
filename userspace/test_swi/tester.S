    .syntax unified       /* Use unified ARM/Thumb syntax */
    .global _start        /* Declare the entry point */

_start:
    svc     #5            /* Invoke software interrupt with number 5 */
    b       .             /* Infinite loop to prevent falling off the end */
