.globl set_pgd
set_pgd:
    msr ttbr0_el1, x0   // load our parameter into the translation table base register for EL1 translation
    tlbi vmalle1is      // invalidate the translation lookaside buffer for all stage 1 translations for EL1
    DSB ISH             // data synchronization barrier to ensure everything is committed
    isb                 // instruction synchronization barrier as well to ensure all instructions see the changes
    ret
