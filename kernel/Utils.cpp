#include "Utils.h"

#include <bit>
#include <cstdint>
#include "PointerTypes.h"

namespace MemoryMappedIO
{
    void Put32(VirtualPtr const aAddress, uint32_t const aData)
    {
        auto volatile* pregister = std::bit_cast<uint32_t*>(aAddress.GetAddress());
        *pregister = aData;
    }

    uint32_t Get32(VirtualPtr const aAddress)
    {
        auto volatile* pregister = std::bit_cast<uint32_t*>(aAddress.GetAddress());
        return *pregister;
    }
}

namespace Timing
{
    void Delay(uint64_t const aCount)
    {
        for (auto i = 0U; i < aCount; ++i)
        {
            // make sure the compiler doesn't optimize out the decrementing so that this is
            // an actual cycle count delay
            // NOLINTNEXTLINE(hicpp-no-assembler)
            asm volatile("nop");
        }
    }

    uint32_t GetSystemCounterClockFrequencyHz()
    {
        // Lint check here is wrong, because it doesn't understand the asm output directive
        // NOLINTNEXTLINE(misc-const-correctness)
        uint64_t frequency = 0;
        
        // NOLINTNEXTLINE(hicpp-no-assembler)
        asm ("mrs %0, CNTFRQ_EL0"
            :"=r"(frequency) // output
            : // no inputs
            : // no clobbered registers
        );

        return static_cast<uint32_t>(frequency);
    }
}