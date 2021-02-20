#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <cstdint>

namespace Timer
{
    // Function returns whether to let the timer continue running or not
    using CallbackFunctionPtr = bool(*)(const void* apParam);

    /**
     * Set up the timer to fire repeatedly with a certain interval and trigger the specified callback. Any existing
     * callback will be overwritten.
     * 
     * @param aIntervalMS Amount of time between callbacks firing in milliseconds
     * @param apCallback Function to triggers when the interrupt fires
     * @param apParam Parameter to send to the function
     */
    void RegisterCallback(uint32_t aIntervalMS, CallbackFunctionPtr apCallback, const void* apParam);

    /**
     * Handle an interrupt from the timer
     */
    void HandleIRQ();
}

namespace LocalTimer
{
    // Function returns whether to let the timer continue running or not
    using CallbackFunctionPtr = bool(*)(const void* apParam);

    /**
     * Set up the timer to fire repeatedly with a certain interval and trigger the specified callback. Any existing
     * callback will be overwritten.
     * 
     * @param aInterval Amount of time between callbacks firing in milliseconds
     * @param apCallback Function to triggers when the interrupt fires
     * @param apParam Parameter to send to the function
     */
    void RegisterCallback(uint32_t aIntervalMS, CallbackFunctionPtr apCallback, const void* apParam);

    /**
     * Handle an interrupt from the timer
     */
    void HandleIRQ();
}

#endif // KERNEL_TIMER_H