#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <cstdint>

namespace Scheduler
{
    /**
     * Initializes the scheduler on the CPU timer
     */
    void InitTimer();

    /**
     * Voluntarily give up the CPU and schedule another task to run
     */
    void Schedule();

    using ProcessFunctionPtr = void(*)(const void* apParam);

    /**
     * Create a new process with the specified function and parameter
     * 
     * @param apProcessFn The process function to schedule
     * @param apParam The parameter to pass to the function
     * @return True on success
     */
    bool CreateProcess(ProcessFunctionPtr apProcessFn, const void* apParam);
}

#endif // KERNEL_SCHEDULER_H