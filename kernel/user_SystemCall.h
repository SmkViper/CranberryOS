#ifndef KERNEL_USER_SYSTEM_CALL_H
#define KERNEL_USER_SYSTEM_CALL_H

#include <cstdint>

namespace SystemCall
{
    /**
     * Write a string to UART
     * 
     * @param apString The string to write
     */
    void Write(const char* apString);

    /**
     * Fork the current process
     * 
     * @return 0 in the child pricess, or the process ID in the parent process. Negative for any error
     */
    int32_t Fork();

    /**
     * Exits the calling process
     */
    void Exit();
}

#endif // KERNEL_USER_SYSTEM_CALL_H