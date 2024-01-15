#include "Utils.h"

#include <cstdint>
#include "PointerTypes.h"

namespace MemoryMappedIO
{
    void Put32(VirtualPtr const aAddress, uint32_t const aData)
    {
        volatile auto pregister = reinterpret_cast<uint32_t*>(aAddress.GetAddress());
        *pregister = aData;
    }

    uint32_t Get32(VirtualPtr const aAddress)
    {
        volatile auto pregister = reinterpret_cast<uint32_t*>(aAddress.GetAddress());
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