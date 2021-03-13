// Included from assembly, so can't have anything fancy in here

#ifndef KERNEL_ARM_EXCEPTION_VECTOR_DEFINES_H
#define KERNEL_ARM_EXCEPTION_VECTOR_DEFINES_H

#define STACK_FRAME_SIZE        272     // we store 17 pairs of 8 byte registers on the stack (17 * 8 * 2)
#define STACK_X0_OFFSET         0       // x0 is stored on the top

// Various values passed to the "invalid exception" handler so it knows which one triggered

#define SYNC_INVALID_EL1t       0
#define IRQ_INVALID_EL1t        1
#define FIQ_INVALID_EL1t        2
#define ERROR_INVALID_EL1t      3

#define SYNC_INVALID_EL1h       4
#define IRQ_INVALID_EL1h        5
#define FIQ_INVALID_EL1h        6
#define ERROR_INVALID_EL1h      7

#define SYNC_INVALID_EL0_64     8
#define IRQ_INVALID_EL0_64      9
#define FIQ_INVALID_EL0_64      10
#define ERROR_INVALID_EL0_64    11

#define SYNC_INVALID_EL0_32     12
#define IRQ_INVALID_EL0_32      13
#define FIQ_INVALID_EL0_32      14
#define ERROR_INVALID_EL0_32    15

#define SYNC_ERROR              16
#define SYSCALL_ERROR           17

#endif // KERNEL_ARM_EXCEPTION_VECTOR_DEFINES_H