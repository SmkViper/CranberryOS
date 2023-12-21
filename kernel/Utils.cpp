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

        return static_cast<uint32_t>(frequency);
    }
}