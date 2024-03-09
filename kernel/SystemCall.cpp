#include <bit>
#include "MiniUart.h"
#include "Scheduler.h"

namespace
{
    /**
     * System call to write a string to the mini UART
     * 
     * @param apBuff The string to write
     */
    void SystemCallWrite(const char* const apBuff)
    {
        MiniUART::SendString(apBuff);
    }

    /**
     * System call to fork the current process
     * 
     * @return 0 in the child process, or the new process ID in the parent process. Negative for any error
     */
    int SystemCallFork()
    {
        return Scheduler::CopyProcess(0 /* no flags */, nullptr /* no starting point */, nullptr /* no parameter */);
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
        std::bit_cast<const void*>(&SystemCallWrite),
        std::bit_cast<const void*>(&SystemCallFork),
        std::bit_cast<const void*>(&SystemCallExit)
    };
}