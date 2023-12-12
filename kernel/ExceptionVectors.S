#include "ARM/ExceptionVectorDefines.h"
#include "ARM/RegisterDefines.h"
#include "SystemCallDefines.h"

// Helper for handling exceptions we don't actually handle by spitting out an error
.macro handle_invalid_entry el, type
    kernel_entry \el
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
.macro kernel_entry el
    sub     sp, sp, #STACK_FRAME_SIZE
    stp     x0, x1, [sp, #16 * 0]   // Store x0 and x1 at the stack pointer, offset by 16 * 0
    stp     x2, x3, [sp, #16 * 1]   // Store x0 and x1 at the stack pointer, offset by 16 * 1
    stp     x4, x5, [sp, #16 * 2]   // etc...
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
    // x30 saved below, so we can pack in more information (because we save 2 registers at a time to preserve stack
    // pointer alignment)

    // now copy additional state into x21, x22, and x23 for saving

    // x21 gets the stack pointer for the exception level
    .if     \el == 0
    mrs     x21, sp_el0
    .else
    add     x21, sp, #STACK_FRAME_SIZE
    .endif
    
    // x22 and x23 get the exception link and processor state registers in case we switch tasks while processing an
    // exception
    mrs     x22, elr_el1
    mrs     x23, spsr_el1

    // finally save off x30 and the other data
    stp     x30, x21, [sp, #16 * 15]
    stp     x22, x23, [sp, #16 * 16]

    .endm

// Helper to restore processor state after handling the exception, returning to exception source
.macro kernel_exit el
    // load the saved off x30 and other processor registers into x21, x22, and x23
    ldp     x22, x23, [sp, #16 * 16]
    ldp     x30, x21, [sp, #16 * 15]

    // if we're going to return to EL0, then restore the el0 stack pointer from where we saved it on the stack
    // (we don't need to do anything special for EL1, since our current sp is the EL1 sp, though it also was saved)
    .if     \el == 0
    msr     sp_el0, x21
    .endif

    // restore the exception link and processor state registers
    msr     elr_el1, x22
    msr     spsr_el1, x23

    // restore the remaining registers from the stack
    ldp     x0, x1, [sp, #16 * 0]   // Load x0 and x1 from the stack pointer, offset by 16 * 0
    ldp     x2, x3, [sp, #16 * 1]   // Load x0 and x1 from the stack pointer, offset by 16 * 1
    ldp     x4, x5, [sp, #16 * 2]   // etc...
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
    add     sp, sp, #STACK_FRAME_SIZE
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

    ventry  sync_el0_64             // Synchronous 64-bit EL0
    ventry  irq_el0_64              // IRQ 64-bit EL0
    ventry  fiq_invalid_el0_64      // FIQ 64-bit EL0
    ventry  error_invalid_el0_64    // Error 64-bit EL0

    ventry  sync_invalid_el0_32     // Synchronous 32-bit EL0
    ventry  irq_invalid_el0_32      // IRQ 32-bit EL0
    ventry  fiq_invalid_el0_32      // FIQ 32-bit EL0
    ventry  error_invalid_el0_32    // Error 32-bit EL0

// EL1t (shared EL0/EL1 stack pointer) interrupts

sync_invalid_el1t:
    handle_invalid_entry    1, SYNC_INVALID_EL1t

irq_invalid_el1t:
    handle_invalid_entry    1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
    handle_invalid_entry    1, FIQ_INVALID_EL1t

error_invalid_el1t:
    handle_invalid_entry    1, ERROR_INVALID_EL1t

// EL1h (seperate EL0/EL1 stack pointer) interrupts

sync_invalid_el1h:
    handle_invalid_entry    1, SYNC_INVALID_EL1h

irq_el1h:
    kernel_entry 1
    bl  handle_irq
    kernel_exit 1

fiq_invalid_el1h:
    handle_invalid_entry    1, FIQ_INVALID_EL1h

error_invalid_el1h:
    handle_invalid_entry    1, ERROR_INVALID_EL1h

// EL0_64 (EL0 in 64-bit mode) interrupts

sync_el0_64:
    // Important: Do not touch registers x0-x7 so that they can be transparently passed to the system call as the first 8 arguments
    kernel_entry 0
    mrs     x25, esr_el1                    // read the syndrome register
    lsr     x24, x25, #ESR_ELx_EC_SHIFT     // shift the value to get the exception class
    cmp     x24, #ESR_ELx_EC_SVC64          // see if it is a SVC instruction from AArch64
    b.eq    el0_svc
    cmp     x24, #ESR_ELx_EC_DABT_LOW       // see if it is a data abort in EL0
    b.eq    el0_da
    handle_invalid_entry    0, SYNC_ERROR  // some unknown synchronous EL0 exception happened

// Setting up some aliases for ease of reading
syscall_count   .req    x25
syscall_number  .req    x26
syscall_table   .req    x27

el0_svc:
    adr     syscall_table, p_sys_call_table_s   // load the pointer to the system call table
    uxtw    syscall_number, w8                  // syscall number was stored in x8
    mov     syscall_count, #SYSCALL_COUNT
    bl      enable_irq
    cmp     syscall_number, syscall_count       // make sure the call is in range
    b.hs    invalid_sys

    ldr     x16, [syscall_table, syscall_number, lsl #3]    // index the table to the requested system call
    blr     x16                                 // and call the requested function
    b       ret_from_syscall
invalid_sys:
    handle_invalid_entry    0, SYSCALL_ERROR
ret_from_syscall:
    bl      disable_irq
    str     x0, [sp, #STACK_X0_OFFSET]          // store x0 onto the stack since it will be bashed by kernel_exit
    kernel_exit 0

el0_da:
    bl      enable_irq
    mrs     x0, far_el1     // load the faulting virtual address into argument 1
    mrs     x1, esr_el1     // load the syndrome information for the fault into argument 2
    bl      do_mem_abort
    cmp     x0, 0           // did do_mem_abort succeed?
    b.eq    1f
    handle_invalid_entry 0, DATA_ABORT_ERROR
1:
    bl disable_irq
    kernel_exit 0

irq_el0_64:
    kernel_entry 0
    bl      handle_irq
    kernel_exit 0

fiq_invalid_el0_64:
    handle_invalid_entry    0, FIQ_INVALID_EL0_64

error_invalid_el0_64:
    handle_invalid_entry    0, ERROR_INVALID_EL0_64

// EL0_32 (EL0 in 32-bit mode) interrupts

sync_invalid_el0_32:
    handle_invalid_entry    0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry    0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry    0, FIQ_INVALID_EL0_32

error_invalid_el0_32:
    handle_invalid_entry    0, ERROR_INVALID_EL0_32

.globl ret_from_fork
ret_from_fork:
    bl      schedule_tail
    cbz     x19, ret_to_user    // if x19 is 0, then we're returning to a user process
    mov     x0, x20             // set up the parameter we're sending to the process function
    blr     x19                 // call the process function, then fall through on return
ret_to_user:
    bl disable_irq
    kernel_exit 0               // will "restore" the state we've prepared for the user process

.globl err_hang
err_hang:   b err_hang  // loop forever
