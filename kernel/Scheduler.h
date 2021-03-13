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

    namespace CreationFlags
    {
        constexpr uint32_t KernelThreadC = 0x1;
    };

    /**
     * Create a new process with the specified function and parameter
     * 
     * @param aCreateFlags Process creation flags
     * @param apProcessFn The process function to schedule (unused for user processes)
     * @param apParam The parameter to pass to the function (unused for user processes)
     * @param apStack The stack pointer for the new process (unused for kernel processes)
     * @return The created process ID, or a negative value on failure
     */
    int32_t CreateProcess(uint32_t aCreateFlags, ProcessFunctionPtr apProcessFn, const void* apParam, void* apStack);

    using UserModeFunctionPtr = void(*)();

    /**
     * Sets this task up as a user process starting at the specified function. Function will be called when calling
     * kernel process function returns (thereby handing control back to ret_from_create)
     * 
     * @param apUserModeFn The function to start executing in user mode
     * @return True on success
     */
    bool MoveToUserMode(UserModeFunctionPtr apUserModeFn);

    /**
     * Exit the current process, cleaning up anything that needs to be (does not return)
     */
    void ExitProcess();
}

#endif // KERNEL_SCHEDULER_H