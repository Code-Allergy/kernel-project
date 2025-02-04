/* linker.ld */
ENTRY(kernel_main)  /* Entry point */

MEMORY
{
    RAM (rwx) : ORIGIN = 0x40120000, LENGTH = 0x4000000 /* 64MB */
}


SECTIONS {
    . = ORIGIN(RAM);

    /* The .text section holds executable code */
    .text : {
        KEEP(*(.text.kernel_main))

        *(.text*)    /* Include all .text sections (code) */
    }
    
    .vectors : {
        KEEP(*(.vectors))  /* Force inclusion of vector section */
        . = ALIGN(32);     /* Mandatory for VBAR alignment */
        _vvectors = .;     /* Create symbol for VBAR initialization */
    } > RAM

    /* The .data section holds initialized data */
    .data : {
        *(.data)    /* Include all .data sections (initialized variables) */
    }

    /* The .bss section holds uninitialized data */
    .bss : {
        *(.bss)     /* Include all .bss sections (uninitialized variables) */
    }



    .stacks : {
        . = ALIGN(8);
        irq_stack_bottom = .;
        . += 0x1000;
        irq_stack_top = .;
        svc_stack_bottom = .;
        . += 0x1000;
        svc_stack_top = .;
    } > RAM

    /* The .stack section for the stack */
    .stack : {
        . = ALIGN(8);
        *(.stack)
    }
}
