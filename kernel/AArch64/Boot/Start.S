// Where the GPU will start execution (put at the correct address via our link.ld file)
.section ".text.boot"

.globl _start

// RPi3 has four processors, all of which start executing this code. The
// mpidr_el1 register contains a variety of processor information, but we're
// only interested in the ID, which exists in the low byte of the register
// value. So mask out the low byte to get the processor ID and if it's 0, boot
// up. Otherwise hang the core by putting it into an infinite loop.
// #TODO: Latest firmware may have changed this so only CPU 0 starts up, with
// the other three sleeping, requiring a memory write to wake each.
//
// IMPORTANT: Do not touch x0-x4, as they contain parameters from the firmware
// that boot_kernel wants. If the data needs to cross a function call boundary,
// use x19-x28 which are non-volatile registers (preserved by callee)
//
// x0 (x20) = 32-bit pointer to the DTB in member (upper 4 bytes may not be
//            cleared)
// x1 (x21) = 0 (reserved)
// x2 (x22) = 0 (reserved)
// x3 (x23) = 0 (reserved)
// x4 (x24) = 32-bit pointer to kernel entry point, (i.e. _start)
_start:
    // using x5 here to avoid corrupting x0-x4 if we're on the master CPU
    mrs     x5, mpidr_el1  // What core are we running on?
    and     x5, x5, #0xFF
    cbz     x5, master
    b       proc_hang

// Infinite loop to hang the other processors with
proc_hang:
    msr     daifset, #2 // Mask out all interrupts
    wfi                 // Wait for an interrupt (go to low power state)
    b       proc_hang

// Main processor starts here
master:
    // Load the start of the .bss segment into x5 and the count into x6 (32
    // bits) then loop and store zero into every 64-bit chunk
    adr     x5, __bss_start
    ldr     w6, =__bss_size_divided_by_8
    cbz     w6, continue_boot
clear_bss:
    str     xzr, [x5], #8       // Stores 0 into *x5 and adds 8
    sub     w6, w6, #1          // Decrement w6
    cbnz    w6, clear_bss       // If w6 is not 0, go to next chunk

continue_boot:
    // Point our stack to the end of the space the linker reserved for it (it
    // grows towards 0). This does run the risk of bashing the kernel image if
    // the stack grows too large, but for now it's fine.
    // #TODO: Either find a better way to do this, or start up a seperate
    // "proper" thread early in the boot process and let this stack go away

    // Loads the offset to the page where _end_of_kernel_stack is
    adrp    x5, _end_of_kernel_stack
    // Add in the offset to the actual _end_of_kernel_stack symbol
    add     x5, x5, :lo12:_end_of_kernel_stack
    mov     sp, x5
    bl      boot_kernel

    b       proc_hang           // hang the processor if boot_kernel ever exits
