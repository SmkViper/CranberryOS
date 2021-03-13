#include "SystemCallDefines.h"

.globl call_sys_uart_write
call_sys_uart_write:
    mov w8, #SYS_UART_WRITE_INDEX
    svc #0
    ret

.globl call_sys_allocate_page
call_sys_allocate_page:
    mov w8, #SYS_ALLOCATE_PAGE_INDEX
    svc #0
    ret

.globl call_sys_create_process
call_sys_create_process:
    /* Save off the arguments for the child */
    mov x10, x0     // function
    mov x11, x1     // argument
    mov x12, x2     // stack pointer

    mov x0, x2      // move stack pointer into argument 0
    mov w8, #SYS_CREATE_PROCESS_INDEX
    svc #0

    cmp x0, #0      // return value of 0 means we're the new process, otherwise we're the caller
    beq process_start
    ret

process_start:
    mov x29, 0

    mov x0, x11     // pull the argument out of where we saved it
    blr x10         // call the process function

    // now that the process has exited, call exit
    mov x8, #SYS_EXIT_INDEX
    svc #0
    // not expected to return from exit

.globl call_sys_exit
call_sys_exit:
    mov x8, #SYS_EXIT_INDEX
    svc #0
    ret
    