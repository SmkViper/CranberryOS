#include "user_SystemCall.h"

extern "C"
{
    // Actual system calls in user_SystemCall.S
    void call_sys_write(const char* apString);
    int32_t call_sys_fork();
    void call_sys_exit();
}

namespace SystemCall
{
    __attribute__((section(".text.user")))
    void Write(const char* const apString)
    {
        call_sys_write(apString);
    }

    __attribute__((section(".text.user")))
    int32_t Fork()
    {
        return call_sys_fork();
    }

    __attribute__((section(".text.user")))
    void Exit()
    {
        call_sys_exit();
    }
}