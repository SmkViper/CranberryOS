#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

namespace Timer
{
    /**
     * Initialize the timer
     */
    void Init();

    /**
     * Handle an interrupt from the timer
     */
    void HandleIRQ();
}

#endif // KERNEL_TIMER_H