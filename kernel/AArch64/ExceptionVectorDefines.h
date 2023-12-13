// Included from assembly, so can't have anything fancy in here

#ifndef KERNEL_AARCH64_EXCEPTION_VECTOR_DEFINES_H
#define KERNEL_AARCH64_EXCEPTION_VECTOR_DEFINES_H

#define STACK_FRAME_SIZE        272     // we store 17 pairs of 8 byte registers on the stack (17 * 8 * 2)
#define STACK_X0_OFFSET         0       // x0 is stored on the top

// Various values passed to the "invalid exception" handler so it knows which one triggered

#define SYNC_INVALID_EL1t       0
#define IRQ_INVALID_EL1t        1
#define FIQ_INVALID_EL1t        2
#define ERROR_INVALID_EL1t      3

#define SYNC_INVALID_EL1h       4
#define FIQ_INVALID_EL1h        5
#define ERROR_INVALID_EL1h      6

#define FIQ_INVALID_EL0_64      7
#define ERROR_INVALID_EL0_64    8

#define SYNC_INVALID_EL0_32     9
#define IRQ_INVALID_EL0_32      10
#define FIQ_INVALID_EL0_32      11
#define ERROR_INVALID_EL0_32    12

#define SYNC_ERROR              13
#define SYSCALL_ERROR           14
#define DATA_ABORT_ERROR        15

#endif // KERNEL_AARCH64_EXCEPTION_VECTOR_DEFINES_H