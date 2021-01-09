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
    sub     sp, sp, #SAVE_FRAME_SIZE    // Make sure we have enough space on the stack for the registers
    stp     x0, x1, [sp, #16 * 0]       // Store x0 and x1 to the stack pointer, offset by 0 (each is 8 bytes)
    stp     x2, x3, [sp, #16 * 1]       // Store x2 and x3 to the stack pointer, offset by 16 (each is 8 bytes)
    stp     x4, x5, [sp, #16 * 2]       // etc...
    stp     x6, x7, [sp, #16 * 3]
    stp     x8, x9, [sp, #16 * 4]
    stp     x10, x11, [sp, #16 * 5]
    stp     x12, x13, [sp, #16 * 6]
    stp     x14, x15, [sp, #16 * 7]
    stp     x16, x17, [sp, #16 * 8]
    stp     x18, x19, [sp, #16 * 9]
    stp     x20, x21, [sp, #16 * 10]
    stp     x22, x23, [sp, #16 * 11]
    stp     x24, x25, [sp, #16 * 12]
    stp     x26, x27, [sp, #16 * 13]
    stp     x28, x29, [sp, #16 * 14]
    str     x30, [sp, #16 * 15]
    .endm

// Helper to restore processor state after handling the exception, returning to exception source
.macro kernel_exit
    ldp     x0, x1, [sp, #16 * 0]       // Load x0 and x1 from the stack pointer, offset by 0 (each is 8 bytes)
    ldp     x2, x3, [sp, #16 * 1]       // Load x2 and x3 from the stack pointer, offset by 16 (each is 8 bytes)
    ldp     x4, x5, [sp, #16 * 2]       // etc...
    ldp     x6, x7, [sp, #16 * 3]
    ldp     x8, x9, [sp, #16 * 4]
    ldp     x10, x11, [sp, #16 * 5]
    ldp     x12, x13, [sp, #16 * 6]
    ldp     x14, x15, [sp, #16 * 7]
    ldp     x16, x17, [sp, #16 * 8]
    ldp     x18, x19, [sp, #16 * 9]
    ldp     x20, x21, [sp, #16 * 10]
    ldp     x22, x23, [sp, #16 * 11]
    ldp     x24, x25, [sp, #16 * 12]
    ldp     x26, x27, [sp, #16 * 13]
    ldp     x28, x29, [sp, #16 * 14]
    ldr     x30, [sp, #16 * 15]
    add     sp, sp, #SAVE_FRAME_SIZE    // Restore the old stack pointer value
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
