#include "ARM/MMUDefines.h"
#include "ARM/SystemRegisters.h"

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
// IMPORTANT: the registers x0 through x4 contain data from the firmware for
// the kernel. (i.e. things like the device tree blob), so we need to preserve
// their values, and restore them right before calling into kernel_main
//
// we do this by shoving their values into x20 and higher, since x19-x28 are
// "callee-saved registers" according to the procedure call standard, and so
// any code we call into is required to preserve them for us if it uses them.
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
    // Preserve our parameters from the firmware
    mov     x20, x0
    mov     x21, x1
    mov     x22, x2
    mov     x23, x3
    mov     x24, x4

    // Load the start of the .bss segment into x0 and the count into x1 (32
    // bits) then loop and store zero into every 64-bit chunk
    adr     x0, __bss_start
    ldr     w1, =__bss_size_divided_by_8
    cbz     w1, switch_exception_level
clear_bss:
    str     xzr, [x0], #8       // Stores 0 into *x0 and adds 8
    sub     w1, w1, #1          // Decrement w1
    cbnz    w1, clear_bss

switch_exception_level:
    // Point our stack to start just before our code (it grows towards 0).
    // #TODO: There's likely a better way to do this, hopefully in a way that doesn't risk stomping anything before our
    // kernel start point, but we'll have to investigate the memory mapping on boot.
    adrp    x0, _start          // Loads the offset to the page where _start is
    add     x0, x0, :lo12:_start // Add in the offset to the actual _start symbol
    mov     sp, x0
    bl      switch_to_el1
    bl      create_page_tables

    // Update the stack pointer to point at the virtual address instead of a physical one
    mov     x0, #VA_START
    add     sp, x0, #LOW_MEMORY

    // Update the table translation register to point at our page directory
    adrp    x0, __pg_dir
    msr     ttbr1_el1, x0
    // Set ttbr0 to the same page table. At the moment it's an identity table, so we should be ok to reuse it. It's
    // needed, however, because otherwise the PC pointing at the "br x2" instruction after we turn on the MMU will
    // fault because the PC register is still pointing in the lower half. (For some reason, real hardware does not seem
    // to need this - perhaps due to instruction caches that QEMU isn't emulating?)
    msr     ttbr0_el1, x0

    // Set up the translation control register for stage 1 of the EL1/0 translation regime
    ldr     x0, =(TCR_VALUE)
    msr     tcr_el1, x0

    // Set up the MAIR register with the flags we use for memory types
    ldr     x0, =(MAIR_VALUE)
    msr     mair_el1, x0

    // Load the absolute address of our kernel main function into x9 BEFORE we turn on the MMU (x9 ensures we don't
    // stomp on any parameters passed in x0-x7, and the XR register which is x8)
    ldr     x9, =kmain

    // Now turn on the MMU
    mov     x0, #SCTLR_MMU_ENABLED
    msr     sctlr_el1, x0

    // And jump to the kmain address we stored previously, after restoring the firmware parameters from x20+
    mov     x0, x20
    mov     x1, x21
    mov     x2, x22
    mov     x3, x23
    mov     x4, x24
    blr     x9
    b       proc_hang           // hang the processor if kmain ever exits
