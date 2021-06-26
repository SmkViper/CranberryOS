#include "SystemCallDefines.h"

.section ".text.user"

.globl call_sys_write
call_sys_write:
    mov w8, #SYS_WRITE_INDEX
    svc #0
    ret

.globl call_sys_fork
call_sys_fork:
    mov w8, #SYS_FORK_INDEX
    svc #0
    ret

.globl call_sys_exit
call_sys_exit:
    mov x8, #SYS_EXIT_INDEX
    svc #0
    ret
    