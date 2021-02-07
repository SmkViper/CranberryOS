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

    uint32_t GetSystemCounterClockFrequencyHz()
    {
        uint64_t frequency = 0;
        asm ("mrs %0, CNTFRQ_EL0"
            :"=r"(frequency) // output
            : // no inputs
            : // no clobbered registers
        );

        return frequency;
    }
}

namespace CPU
{
    uint64_t GetExceptionLevel()
    {
        uint64_t currentELValue = 0;
        asm ("mrs %0, CurrentEL"
            :"=r"(currentELValue) // output
            : // no inputs
            : // no clobbered registers
        );
        // CurrentEL register stores the exception level in bits 2 and 3, so shift away the lower two bits
        // and mask the rest
        return (currentELValue >> 2) & 0x3;
    }
}