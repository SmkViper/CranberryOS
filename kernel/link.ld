/* Let the linker know what the entry point is going to be, so it can position it at the start */
ENTRY(_start)

/* 
    Once we turn on the MMU, the kernel will be remapped to be relative to this address. We seperate it out like this
    so we can use the AT(...) directive to tell the linker what the physical addresses will be (by subtracting this
    value from the virtual address that the linker is calculating for each section)
*/
KERNEL_MAPPING_BASE = 0xffff000000000000;

SECTIONS
{
    /* 0x00080000 is the physical address the Raspberry Pi GPU will load the kernel at */
    . = KERNEL_MAPPING_BASE + 0x00080000;

    /*
        Make sure .text.boot section is at the very front, and that the linker knows it is the root for doing garbage
        collection, since it's the entry point of the kernel.
    */
    .text.boot : AT (ADDR(.text.boot) - KERNEL_MAPPING_BASE) {
        KEEP(*(.text.boot)) /* boot code section */
    }

    /*
        As a hack until we can load programs via a file system, any file starting with "user" will be considered part
        of the user process, and be put in this segment between user_begin and user_end. That way we can just memcpy
        to "load" our process
    */
    . = ALIGN(0x00001000);
    __user_start = .;
    .text.user : AT (ADDR(.text.user) - KERNEL_MAPPING_BASE) {
        CMakeFiles/kernel8.elf.dir/user_* (.text)
        *(.text.user)
    }
    .rodata.user : AT (ADDR(.rodata.user) - KERNEL_MAPPING_BASE) {
        CMakeFiles/kernel8.elf.dir/user_* (.rodata.*)
    }
    .data.user : AT (ADDR(.data.user) - KERNEL_MAPPING_BASE) {
        CMakeFiles/kernel8.elf.dir/user_* (.data)
    }
    .bss.user : AT (ADDR(.bss.user) - KERNEL_MAPPING_BASE) {
        CMakeFiles/kernel8.elf.dir/user_* (.bss)
    }
    __user_end = .;

    .text : AT (ADDR(.text) - KERNEL_MAPPING_BASE) {
        *(.text .text.*) /* remaining code sections */
    }

    .rodata : AT (ADDR(.rodata) - KERNEL_MAPPING_BASE) {
        *(.rodata .rodata.*) /* read-only data sections */
    }

    /* .init_array is a list of pointers to constructors for static variables - pointer alignment */
    .init_array ALIGN(0x8) : AT (ADDR(.init_array) - KERNEL_MAPPING_BASE) {
        __init_start = .;
        KEEP(*(SORT(.init_array.*))) /* make sure constructor priorities are respected by sorting */
        KEEP(*(.init_array*))
        __init_end = .;
    }

    /* .fini_array is a list of pointers to destructors for static variables - pointer alignment */
    .fini_array ALIGN(0x8) : AT (ADDR(.fini_array) - KERNEL_MAPPING_BASE) {
        __fini_start = .;
        KEEP (*(SORT(.fini_array.*))) /* make sure destructor priorities are respected by sorting */
        KEEP(*(.fini_array*))
        __fini_end = .;
    }

    /* exception handler frame information */
    .eh_frame : AT (ADDR(.eh_frame) - KERNEL_MAPPING_BASE) {
        *(.eh_frame*)
    }

    /* stack unwind information */
    .ARM.exidx : AT (ADDR(.ARM.exidx) - KERNEL_MAPPING_BASE) {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    }

    .data ALIGN(0x00001000) : AT (ADDR(.data) - KERNEL_MAPPING_BASE) {
        *(.data .data.*) /* initialized writable data sections */
    }

    /*
        .bss is data that should be initialized to 0, which the compiler puts into this seperate section to save a bit
        of space in the binary. We set the section to be aligned to a multiple of 8 so we can use the str instruction
        to fill it with zeroes, which requires an 8-byte alignment.
        This section is flagged as no load because not only is there no real data in the section to load, but we're
        going to zero it out anyway during startup.
    */
    .bss ALIGN(0x8) (NOLOAD) : AT (ADDR(.bss) - KERNEL_MAPPING_BASE) {
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