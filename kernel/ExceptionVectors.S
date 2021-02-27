#include "ARM/ExceptionVectorDefines.h"

// Helper for handling exceptions we don't actually handle by spitting out an error
.macro handle_invalid_entry type
    kernel_entry
    mov     x0, #\type      // argument 1: type of exception entry
    mrs     x1, esr_el1     // argument 2: ESL register - exception cause
    mrs     x2, elr_el1     // argument 3: instruction address that generated the exception
    bl      show_invalid_entry_message
    b       err_hang
    .endm

// Tiny helper that fills out table entries by making it a branch to a handler, aligned correctly for the vector table
.macro ventry label
    .align  7               // each entry needs to be at a 0x80 offset from eachother
    b       \label
    .endm

// Helper to save off all processor state before handling the exception
.macro kernel_entry
    stp     x29, x30, [sp, #-16]!   // Store x29 and x30 to the stack, pre-decrementing sp by 16
    stp     x27, x28, [sp, #-16]!   // Store x27 and x28 to the stack, pre-decrementing sp by 16
    stp     x25, x26, [sp, #-16]!   // etc...
    stp     x23, x24, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x19, x20, [sp, #-16]!
    stp     x17, x18, [sp, #-16]!
    stp     x15, x16, [sp, #-16]!
    stp     x13, x14, [sp, #-16]!
    stp     x11, x12, [sp, #-16]!
    stp     x9, x10, [sp, #-16]!
    stp     x7, x8, [sp, #-16]!
    stp     x5, x6, [sp, #-16]!
    stp     x3, x4, [sp, #-16]!
    stp     x1, x2, [sp, #-16]!

    // need to save off exception link and processor state registers as well in case we switch tasks while processing
    // an exception
    mrs     x22, elr_el1
    mrs     x23, spsr_el1

    stp     x0, x22, [sp, #-16]!
    str     x23, [sp, #-16]!
    .endm

// Helper to restore processor state after handling the exception, returning to exception source
.macro kernel_exit
    ldr     x23, [sp], #16      // Load spsr_el1 from the stack, post-incrementing sp by 16
    ldp     x0, x22, [sp], #16  // Load x0 and elr_el1 from the stack, post-incrementing sp by 16
    
    // restore the exception link and processor state registers
    msr     elr_el1, x22
    msr     spsr_el1, x23

    ldp     x1, x2, [sp], #16   // Load x1 and x2 from the stack, post-incrementing sp by 16
    ldp     x3, x4, [sp], #16   // Load x3 and x4 from the stack, post-incrementing sp by 16
    ldp     x5, x6, [sp], #16   // etc...
    ldp     x7, x8, [sp], #16
    ldp     x9, x10, [sp], #16
    ldp     x11, x12, [sp], #16
    ldp     x13, x14, [sp], #16
    ldp     x15, x16, [sp], #16
    ldp     x17, x18, [sp], #16
    ldp     x19, x20, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x27, x28, [sp], #16
    ldp     x29, x30, [sp], #16
    eret
    .endm

// Exception vector table
.align  11
.globl  vectors
vectors:
    ventry  sync_invalid_el1t       // Synchronous EL1t (stack pointer shared with EL0 and EL1)
    ventry  irq_invalid_el1t        // IRQ EL1t
    ventry  fiq_invalid_el1t        // FIQ EL1t
    ventry  error_invalid_el1t      // Error EL1t

    ventry  sync_invalid_el1h       // Synchronous EL1h (stack pointer for EL0 and EL1 are seperate)
    ventry  irq_el1h                // IRQ EL1h
    ventry  fiq_invalid_el1h        // FIQ EL1h
    ventry  error_invalid_el1h      // Error EL1h

    ventry  sync_invalid_el0_64     // Synchronous 64-bit EL0
    ventry  irq_invalid_el0_64      // IRQ 64-bit EL0
    ventry  fiq_invalid_el0_64      // FIQ 64-bit EL0
    ventry  error_invalid_el0_64    // Error 64-bit EL0

    ventry  sync_invalid_el0_32     // Synchronous 32-bit EL0
    ventry  irq_invalid_el0_32      // IRQ 32-bit EL0
    ventry  fiq_invalid_el0_32      // FIQ 32-bit EL0
    ventry  error_invalid_el0_32    // Error 32-bit EL0

// EL1t (shared EL0/EL1 stack pointer) interrupts

sync_invalid_el1t:
    handle_invalid_entry    SYNC_INVALID_EL1t

irq_invalid_el1t:
    handle_invalid_entry    IRQ_INVALID_EL1t

fiq_invalid_el1t:
    handle_invalid_entry    FIQ_INVALID_EL1t

error_invalid_el1t:
    handle_invalid_entry    ERROR_INVALID_EL1t

// EL1h (seperate EL0/EL1 stack pointer) interrupts

sync_invalid_el1h:
    handle_invalid_entry    SYNC_INVALID_EL1h

irq_el1h:
    kernel_entry
    bl  handle_irq
    kernel_exit

fiq_invalid_el1h:
    handle_invalid_entry    FIQ_INVALID_EL1h

error_invalid_el1h:
    handle_invalid_entry    ERROR_INVALID_EL1h

// EL0_64 (EL0 in 64-bit mode) interrupts

sync_invalid_el0_64:
    handle_invalid_entry    SYNC_INVALID_EL0_64

irq_invalid_el0_64:
    handle_invalid_entry    IRQ_INVALID_EL0_64

fiq_invalid_el0_64:
    handle_invalid_entry    FIQ_INVALID_EL0_64

error_invalid_el0_64:
    handle_invalid_entry    ERROR_INVALID_EL0_64

// EL0_32 (EL0 in 32-bit mode) interrupts

sync_invalid_el0_32:
    handle_invalid_entry    SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry    IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry    FIQ_INVALID_EL0_32

error_invalid_el0_32:
    handle_invalid_entry    ERROR_INVALID_EL0_32

.globl err_hang
err_hang:   b err_hang  // loop forever
