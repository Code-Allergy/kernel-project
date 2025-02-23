#include <stdarg.h>
#include <kernel/printk.h>

void panic(const char *fmt, ...){
     va_list args;

     // Disable interrupts (inline assembly)
     __asm__("cpsid i");
     va_start(args, fmt);

     // TODO dump registers and stack, current process, other debug info
     printk("PANIC: ");
     vprintk(fmt, args);
     printk(" at %s:%d\n", __FILE__, __LINE__);
     printk("System halted.\n");

     while (1) {
         __asm__("wfi");
     }

     va_end(args);
     __builtin_unreachable();
 }
