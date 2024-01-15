#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <cstddef>
#include <cstdint>

namespace Scheduler
{
    struct TaskStruct;

    /**
     * Initializes the scheduler on the CPU timer
     */
    void InitTimer();

    /**
     * Voluntarily give up the CPU and schedule another task to run
     */
    void Schedule();

    using ProcessFunctionPtr = void(*)(void const* apParam);

    namespace CreationFlags
    {
        constexpr uint32_t KernelThreadC = 0x1;
    };

    /**
     * Create a new process with the specified function and parameter
     * 
     * @param aCloneFlags Process clone flags
     * @param apProcessFn The process function to schedule (unused for user processes)
     * @param apParam The parameter to pass to the function (unused for user processes)
     * @return The created process ID, or a negative value on failure
     */
    int32_t CopyProcess(uint32_t aCloneFlags, ProcessFunctionPtr apProcessFn, void const* apParam);

    /**
     * Sets this task up as a user process with the specified memory block and starting point
     * 
     * @param apStart Start of the memory block to copy
     * @param aSize Size of the memory block to copy
     * @param aPC Where to start executing (offset from aStart)
     * @return True on success
     */
    bool MoveToUserMode(void const* apStart, std::size_t aSize, uintptr_t aPC);

    /**
     * Exit the current process, cleaning up anything that needs to be (does not return)
     */
    void ExitProcess();

    /**
     * Obtains the currently running task
     * 
     * @return The currently running task
     */
    TaskStruct& GetCurrentTask();
}

#endif // KERNEL_SCHEDULER_H