SECTIONS
{
    /* Startup code needs to be at 0x80000, since the GPU just loads the file into ram and starts executing there */
    /*. = 0x80000;*/
    /*
        #TODO: Is this the right address? The code I'm looking at originally was at 0x0000 and then changed to this for
        VM support where the kernel starts at this address. But the RPi loads at 0x80000, not 0, so is this still
        valid? Right now this is causing a linker error where it complains we're ouside the available address space
    */
    . = 0xffff000000000000;
    /*
        Make sure .text.boot section is at the very front, and that the linker knows it is the root for doing garbage
        collection, since it's the entry point of the kernel.
    */
    .text : { KEEP(*(.text.boot)) *(.text .text.*) } /* code sections */
    .rodata : { *(.rodata .rodata.*) } /* read-only data sections */

    /* .init_array is a list of pointers to constructors for static variables - pointer alignment */
    .init_array ALIGN(0x8) : {
        __init_start = .;
        KEEP(*(SORT(.init_array.*))) /* make sure constructor priorities are respected by sorting */
        KEEP(*(.init_array*))
        __init_end = .;
    }

    /* .fini_array is a list of pointers to destructors for static variables - pointer alignment */
    .fini_array ALIGN(0x8) : {
        __fini_start = .;
        KEEP (*(SORT(.fini_array.*))) /* make sure destructor priorities are respected by sorting */
        KEEP(*(.fini_array*))
        __fini_end = .;
    }

    /* exception handler frame information */
    .eh_frame : {
        *(.eh_frame*)
    }

    /* stack unwind information */
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    }

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

    /*
        set up space for the page tables. For now we just assume we're mapping 1GB of memory. As such, since we can use
        section mapping (i.e. 2mb sections instead of 4k pages) we don't need a Page Table Entry and can get by with
        a Page Global Directory with one entry pointing at a single Page Upper Directory. This will also have a single
        entry pointing at a single Page Middle Directory, which will then point at all 512 2mb sections.
        That's 3 tables, each 4k in size (hence 3 * (1 << 12))
    */
    . = ALIGN(0x00001000);
    pg_dir = .;
    /* #TODO: This line is causing the linker to freeze if the starting address is 0x80000 for some reason */
    .data.pgd : { . += (3 * (1 << 12)); }
    _end = .;

    /* strip out some general unimportant stuff we don't need */
    /DISCARD/ : { *(.comment) *(.note*) }
}
__bss_size = (__bss_end - __bss_start) >> 3; /* calculate the size as the number of 8 byte chunks */