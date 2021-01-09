.globl irq_vector_init
irq_vector_init:
    adr     x0, vectors
    msr     vbar_el1, x0    // load the vector table into vbar_el1
    ret

.globl enable_irq
enable_irq:
    msr     daifclr, #2
    ret

.globl disable_irq
disable_irq:
    msr     daifclr, #2
    ret
