#include "Utils.h"

namespace MemoryMappedIO
{
    void Put32(uint64_t aAddress, uint32_t aData)
    {
        volatile auto pregister = reinterpret_cast<uint32_t*>(aAddress);
        *pregister = aData;
    }

    uint32_t Get32(uint64_t aAddress)
    {
        volatile auto pregister = reinterpret_cast<uint32_t*>(aAddress);
        return *pregister;
    }
}

namespace Timing
{
    void Delay(uint64_t aCount)
    {
        while (aCount--)
        {
            // make sure the compiler doesn't optimize out the decrementing so that this is
            // an actual cycle count delay
            asm volatile("nop");
        }
    }
}

namespace CPU
{
    uint64_t GetExceptionLevel()
    {
        uint64_t CurrentELValue = 0;
        asm ("mrs %0, CurrentEL"
            :"=r"(CurrentELValue) // output
            : // no inputs
            : // no clobbered registers
        );
        // CurrentEL register stores the exception level in bits 3 and 4, so shift away the lower two bits
        // and mask the rest
        return (CurrentELValue >> 2) & 0x3;
    }
}