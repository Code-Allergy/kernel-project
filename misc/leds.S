.syntax unified
.arm                @ Explicit ARM mode directive

.global _start
_start:
    ldr r0, =0x4804C194    @ GPIO1_SETDATAOUT address (0x4804C000 base + 0x194 offset)
    ldr r1, =0x1E00000     @ Bitmask for LEDs (USR0-USR3: bits 21-24)
    str r1, [r0]           @ Turn on all 4 LEDs

    @ Infinite loop to keep LEDs lit
1:  b 1b
