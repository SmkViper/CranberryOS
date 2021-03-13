#include "ARM/SchedulerDefines.h"

.globl cpu_switch_to
cpu_switch_to:
    // x0 is the previous task pointer
    // x1 is the next task pointer
    mov     x10, #TASK_STRUCT_CONTEXT_OFFSET
    add     x8, x0, x10             // find the context member in the previous task
    mov     x9, sp                  // save off the current stack pointer
    // #TODO: May need to save off additional registers (i.e. SIMD/FP registers?)
    // save off the callee-saved registers, adjusting the pointer into the struct after each pair
    stp     x19, x20, [x8], #16     // store off x19 and x20 into the context of the previous task
    stp     x21, x22, [x8], #16     // store off x21 and x22 into the context of the previous task
    stp     x23, x24, [x8], #16     // etc...
    stp     x25, x26, [x8], #16
    stp     x27, x28, [x8], #16
    stp     x29, x9, [x8], #16      // stores fp and the previously saved sp
    str     x30, [x8]               // stores the link register into pc (where task will resume)
    add     x8, x1, x10     // find the context member in the next task
    // load the callee-saved registers, adjusting the pointer into the struct after each pair
    ldp     x19, x20, [x8], #16     // load x19 and x20 from the context of the next task
    ldp     x21, x22, [x8], #16     // load x21 and x22 from the context of the next task
    ldp     x23, x24, [x8], #16     // etc...
    ldp     x25, x26, [x8], #16
    ldp     x27, x28, [x8], #16
    ldp     x29, x9, [x8], #16      // loads fp and the sp
    ldr     x30, [x8]               // loads the link register from pc (where task will resume after ret)
    mov     sp, x9                  // restore the stack pointer on the task
    ret
