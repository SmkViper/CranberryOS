SECTIONS
{
    /*
        We're using a MMU, so while normally startup code needs to be at 0x80000, we actually want the linker to behave
        as if we're at a much higher address, since that's what we're going to be mapping our memory to. Instead of
        having it be at 0xFFFF'0000'0000'0000 directly, we offset it at 0x80000 since our MMU is going to do a 1-1
        mapping from physical to virtual addresses. So our start address of 0x80000 (dicated by the hardware) will end
        up at 0xFFFF'0000'0008'0000 in virtual address space
    */
    . = 0xffff000000080000;
    /*
        Make sure .text.boot section is at the very front, and that the linker knows it is the root for doing garbage
        collection, since it's the entry point of the kernel.
    */
    .text.boot : { KEEP(*(.text.boot))  } /* boot code section */

    /*
        As a hack until we can load programs via a file system, any file starting with "user" will be considered part
        of the user process, and be put in this segment between user_begin and user_end. That way we can just memcpy
        to "load" our process
    */
    . = ALIGN(0x00001000);
    __user_start = .;
    .text.user : {
        CMakeFiles/kernel8.elf.dir/user_* (.text)
        *(.text.user)
    }
    .rodata.user : { CMakeFiles/kernel8.elf.dir/user_* (.rodata.*) }
    .data.user : { CMakeFiles/kernel8.elf.dir/user_* (.data) }
    .bss.user : { CMakeFiles/kernel8.elf.dir/user_* (.bss) }
    __user_end = .;

    .text : { *(.text .text.*) } /* remaining code sections */

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

    .data ALIGN(0x00001000) : { *(.data .data.*) } /* initialized writable data sections */
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
    . = ALIGN(0x00001000);
    pg_dir = .;
    _end = .;

    /* strip out some general unimportant stuff we don't need */
    /DISCARD/ : { *(.comment) *(.note*) }
}
__bss_size = (__bss_end - __bss_start) >> 3; /* calculate the size as the number of 8 byte chunks */