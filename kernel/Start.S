#include "ARM/SystemRegisters.h"

// Macro to switch the current processor to el1, using the given scratch
// register. Assumes we are in EL2. If there is some way to boot the RPi into
// EL3, then we'll have to add some additional code to move into EL2 first.
.macro armv8_switch_to_el1, xreg1
    // Initialize the processor settings (see SystemRegisters.h)
    ldr     \xreg1, =SCTLR_INIT_VALUE
    msr     sctlr_el1, \xreg1

    ldr     \xreg1, =HCR_EL2_INIT_VALUE
    msr     hcr_el2, \xreg1

    ldr     \xreg1, =SPSR_EL2_INIT_VALUE
    msr     spsr_el2, \xreg1

    // Overwrite the exception return address with the location of the first 1
    // label past this instruction, and then return to it (in EL1)
    adr     \xreg1, 1f
    msr     elr_el2, \xreg1
    eret
1:
    .endm

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
_start:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbz     x0, master
    b       proc_hang

// Infinite loop to hang the other processors with
proc_hang:
    b       proc_hang

// Main processor starts here
master:
    // See if our exception level is 1 already, if it is, skip elevation.
    // Exception level is in bits 2 and 3
    mrs     x0, CurrentEL
    cmp     x0, #(1 << 2)
    beq     1f

    armv8_switch_to_el1 x0

1:
    // Set up the stack pointer to start before our code (it will grow towards
    // 0)
    ldr     x0, =_start
    mov     sp, x0

    // Load the start of the .bss segment into x0 and the count into x1 (32
    // bits) then loop and store zero into every 64-bit chunk
    ldr     x0, =__bss_start
    ldr     w1, =__bss_size
    cbz     w1, start_kmain
clear_bss:
    str     xzr, [x0], #8       // Stores 0 into *x0 and adds 8
    sub     w1, w1, #1          // Decrement w1
    cbnz    w1, clear_bss

start_kmain:
    bl      kmain
    b       proc_hang           // hang the processor if kmain ever exits
