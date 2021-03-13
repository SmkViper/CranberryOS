#ifndef KERNEL_SYSTEM_CALL_H
#define KERNEL_SYSTEM_CALL_H

#include <cstdint>

namespace SystemCall
{
    /**
     * Write a string to UART
     * 
     * @param apString The string to write
     */
    void UARTWrite(const char* apString);

    /**
     * Allocate a page of memory
     * 
     * @return The allocated page, or null if it failed
     */
    void* AllocatePage();

    using ProcessFunctionPtr = void(*)(const void* apParam);

    /**
     * Create a new process
     * 
     * @param apFunction The function the process should start on
     * @param apParam The parameter to pass to the function
     * @param apStackRoot Pointer to memory to use for the process' stack
     * @return The ID of the created process
     */
    int32_t CreateProcess(ProcessFunctionPtr apFunction, const void* apParam, const void* apStackRoot);

    /**
     * Exits the calling process
     */
    void Exit();
}

#endif // KERNEL_SYSTEM_CALL_H