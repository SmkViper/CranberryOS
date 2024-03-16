#ifndef KERNEL_AARCH64_CPU_H
#define KERNEL_AARCH64_CPU_H

#include <cstdint>

namespace AArch64::CPU
{
    enum class ExceptionLevel : uint8_t
    {
        EL0 = 0, // user land
        EL1 = 1, // OS level
        EL2 = 2, // hypervisor
        EL3 = 3  // firmware (secure/insecure world switching)
    };

    /**
     * Obtains the current exception level we're running under
     * 
     * @return The current exception level
     */
    ExceptionLevel GetCurrentExceptionLevel();

    /**
     * Halts the CPU (never returns)
     */
    [[noreturn]] void Halt();
}

#endif // KERNEL_AARCH64_CPU_H