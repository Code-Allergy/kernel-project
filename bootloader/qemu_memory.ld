ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x40010000, LENGTH = 0x100000  /* 1MB */
    RAM (rwx) : ORIGIN = 0x40110000, LENGTH = 0x4000000 /* 64MB */
}

SECTIONS
{
    . = ORIGIN(ROM);
    .text : { *(.text) }

    . = ORIGIN(RAM);
    .data : { *(.data) }
    .bss : { *(.bss COMMON) }
    . = ALIGN(8);
    . = . + 0x1000; /* 4kB of stack memory */
    _stack_start = .;
}
