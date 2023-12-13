#include "CPU.h"

namespace AArch64
{
    namespace CPU
    {
        ExceptionLevel GetCurrentExceptionLevel()
        {
            uint64_t exceptionLevel = 0;

            asm volatile("mrs %[value], CurrentEL" : [value] "=r"(exceptionLevel));

            // The exception level is in bits 2 and 3 of the value in the CurrentEL register, so extract those and
            // convert to our enum
            exceptionLevel = (exceptionLevel >> 2) & 0b11;
            return static_cast<ExceptionLevel>(exceptionLevel);
        }
    }
}