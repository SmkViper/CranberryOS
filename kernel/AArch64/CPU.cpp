#include "CPU.h"

#include <cstdint>

namespace AArch64::CPU
{
    ExceptionLevel GetCurrentExceptionLevel()
    {
        uint64_t exceptionLevel = 0;

        // NOLINTNEXTLINE(hicpp-no-assembler)
        asm volatile("mrs %[value], CurrentEL" : [value] "=r"(exceptionLevel));

        // The exception level is in bits 2 and 3 of the value in the CurrentEL register, so extract those and
        // convert to our enum
        exceptionLevel = (exceptionLevel >> 2U) & 0b11U;
        return static_cast<ExceptionLevel>(exceptionLevel);
    }

    [[noreturn]] void Halt()
    {
        for (;;)
        {
            // wait forever for interupts (which won't happen at this point in the boot process)
            // NOLINTNEXTLINE(hicpp-no-assembler)
            asm volatile("wfi");
        }
    }
}