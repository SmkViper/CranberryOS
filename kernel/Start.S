// Where the GPU will start execution (put at the correct address via our link.ld file)
.section ".text.boot"

.globl _start

// RPi3 has four processors, all of which start executing this code. The
// mpidr_el1 register contains a variety of processor information, but we're
// only interested in the ID, which exists in the low byte of the register
// value. So mask out the low byte to get the processor ID and if it's 0, boot
// up. Otherwise hang the core by putting it into an infinite loop.
// TODO: Latest firmware may have changed this so only CPU 0 starts up, with
// the other three sleeping, requiring a memory write to wake each.
//
// IMPORTANT: Do not touch x0-x4, as they contain parameters from the firmware
// that boot_kernel wants. Use x19-x28 which are non-volatile registers
// (preserved by callee)
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
    // Load the start of the .bss segment into x0 and the count into x1 (32
    // bits) then loop and store zero into every 64-bit chunk
    adr     x19, __bss_start
    ldr     w1, =__bss_size_divided_by_8
    cbz     w1, continue_boot
clear_bss:
    str     xzr, [x19], #8       // Stores 0 into *x0 and adds 8
    sub     w1, w1, #1          // Decrement w1
    cbnz    w1, clear_bss

continue_boot:
    // Point our stack to start just before our code (it grows towards 0).
    // #TODO: There's likely a better way to do this, hopefully in a way that
    // doesn't risk stomping anything before our kernel start point, but we'll
    // have to investigate the memory mapping on boot. Old version used
    // LOW_MEMORY for the stack root, which covered the first 4MB of address
    // space (and therefore ran the risk of bashing the kernel image)
    adrp    x19, _start          // Loads the offset to the page where _start is
    add     x19, x19, :lo12:_start // Add in the offset to the actual _start symbol
    mov     sp, x19
    bl      boot_kernel

    b       proc_hang           // hang the processor if book_kernel ever exits
