#include "SystemCall.h"

#include "MemoryManager.h"
#include "MiniUart.h"
#include "Scheduler.h"
#include "SystemCallDefines.h"

namespace
{
    /**
     * System call to write a string to the mini UART
     * 
     * @param apBuff The string to write
     */
    void SystemCallUARTWrite(const char* const apBuff)
    {
        MiniUART::SendString(apBuff);
    }

    /**
     * System call to allocate a page of memory
     * 
     * @return The address to the new page of memory
     */
    void* SystemCallAllocatePage()
    {
        return MemoryManager::GetFreePage();
    }

    /**
     * System call to create a new process with a stack at the specified memory address
     * 
     * @param apStack Where to put the stack
     * @return The process ID for the new process
     */
    int SystemCallCreateProcess(void* const apStack)
    {
        return Scheduler::CreateProcess(0, nullptr, nullptr, apStack);
    }

    /**
     * System call to exit the process
     */
    void SystemCallExit()
    {
        Scheduler::ExitProcess();
    }
}

extern "C"
{
    // ExceptionVector.S uses this along with the index passed in x8 to call the right system call
    extern const void* const p_sys_call_table_s[] = {
        reinterpret_cast<const void*>(&SystemCallUARTWrite),
        reinterpret_cast<const void*>(&SystemCallAllocatePage),
        reinterpret_cast<const void*>(&SystemCallCreateProcess),
        reinterpret_cast<const void*>(&SystemCallExit)
    };

    // Actual system calls in SystemCall.S
    void call_sys_uart_write(const char* apString);
    void* call_sys_allocate_page();
    int32_t call_sys_create_process(SystemCall::ProcessFunctionPtr apFunction, const void* apParam, const void* apStackRoot);
    void call_sys_exit();
}

namespace SystemCall
{
    void UARTWrite(const char* const apString)
    {
        call_sys_uart_write(apString);
    }

    void* AllocatePage()
    {
        return call_sys_allocate_page();
    }

    int32_t CreateProcess(ProcessFunctionPtr apFunction, const void* apParam, const void* apStackRoot)
    {
        return call_sys_create_process(apFunction, apParam, apStackRoot);
    }

    void Exit()
    {
        call_sys_exit();
    }
}