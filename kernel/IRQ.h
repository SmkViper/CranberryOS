#ifndef KERNEL_IRQ_H
#define KERNEL_IRQ_H

// Defined in assembly
extern "C"
{
    /**
     * Initialize the vector table
     */
    void irq_vector_init();

    /**
     * Enable interrupts
     */
    void enable_irq();

    /**
     * Disable interrupts
     */
    void disable_irq();
}

#endif // KERNEL_IRQ_H