#include <stdarg.h>
#include <kernel/printk.h>

void _panic(const char* file, const char* line, const char *fmt, ...){
     va_list args;

     // Disable interrupts (inline assembly)
     __asm__("cpsid i");
     va_start(args, fmt);

     // TODO dump registers and stack, current process, other debug info
     printk("PANIC: ");
     vprintk(fmt, args);
     printk(" at %s:%d\n", file, line);
     printk("System halted.\n");

     while (1) {
         __asm__("wfi");
     }

     va_end(args);
     __builtin_unreachable();
 }
