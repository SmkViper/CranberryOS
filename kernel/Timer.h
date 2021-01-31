#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <cstdint>

namespace Timer
{
    using CallbackFunctionPtr = void(*)(const void* apParam);

    /**
     * Set up the timer to fire after a certain amount of time and trigger the specified callback. Any existing
     * callback will be overwritten.
     * 
     * @param aInterval Amount of time to pass until the callback fires (TODO: See what unit this in)
     * @param apCallback Function to triggers when the interrupt fires
     * @param apParam Parameter to send to the function
     */
    void RegisterCallback(uint32_t aInterval, CallbackFunctionPtr apCallback, const void* apParam);

    /**
     * Handle an interrupt from the timer
     */
    void HandleIRQ();
}

namespace LocalTimer
{
    using CallbackFunctionPtr = void(*)(const void* apParam);

    /**
     * Set up the timer to fire after a certain amount of time and trigger the specified callback. Any existing
     * callback will be overwritten.
     * 
     * @param aInterval Amount of time to pass until the callback fires (TODO: See what unit this in, it's a smaller
     * unit of time than the global timer)
     * @param apCallback Function to triggers when the interrupt fires
     * @param apParam Parameter to send to the function
     */
    void RegisterCallback(uint32_t aInterval, CallbackFunctionPtr apCallback, const void* apParam);

    /**
     * Handle an interrupt from the timer
     */
    void HandleIRQ();
}

#endif // KERNEL_TIMER_H