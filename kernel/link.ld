SECTIONS
{
    /* Startup code needs to be at 0x80000, since the GPU just loads the file into ram and starts executing there */
    . = 0x80000;
    /*
        Make sure .text.boot section is at the very front, and that the linker knows it is the root for doing garbage
        collection, since it's the entry point of the kernel.
    */
    .text : { KEEP(*(.text.boot)) *(.text .text.*) } /* code sections */
    .rodata : { *(.rodata .rodata.*) } /* read-only data sections */
    .data : { *(.data .data.*) } /* initialized writable data sections */
    /*
        .bss is data that should be initialized to 0, which the compiler puts into this seperate section to save a bit
        of space in the binary. We set the section to be aligned to a multiple of 8 so we can use the str instruction
        to fill it with zeroes, which requires an 8-byte alignment.
        This section is flagged as no load because not only is there no real data in the section to load, but we're
        going to zero it out anyway during startup.
    */
    .bss ALIGN(0x8) (NOLOAD) : {
        /* record the start and end of the section so the assembly can know what memory region to zero out */
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;

    /* strip out some general unimportant stuff we don't need */
    /DISCARD/ : { *(.comment) *(.note*) }
}
__bss_size = (__bss_end - __bss_start) >> 3; /* calculate the size as the number of 8 byte chunks */