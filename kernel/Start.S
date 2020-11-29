// Where the GPU will start execution (put at the correct address via our link.ld file)
.section ".text.boot"

.globl _start

// RPi3 has four processors, all of which start executing this code. The
// mpidr_el1 register contains a variety of processor information, but we're
// only interested in the ID, which exists in the low byte of the register
// value. So mask out the low byte to get the processor ID and if it's 0, boot
// up. Otherwise hang the core by putting it into an infinite loop.
_start:
    mrs     x0, mpidr_el1
    and     x0, x0,#0xFF
    cbz     x0, master
    b       proc_hang

// Infinite loop to hang the other processors with
proc_hang:
    b       proc_hang

master:
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
    sub     w1, w1, #1          // Decrement x1
    cbnz    w1, clear_bss

start_kmain:
    bl      kmain
