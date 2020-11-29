#include "Utils.h"

namespace MemoryMappedIO
{
    void Put32(unsigned long aAddress, unsigned int aData)
    {
        volatile auto pregister = reinterpret_cast<unsigned int*>(aAddress);
        *pregister = aData;
    }

    unsigned int Get32(unsigned long aAddress)
    {
        volatile auto pregister = reinterpret_cast<unsigned int*>(aAddress);
        return *pregister;
    }
}

namespace Timing
{
    void Delay(unsigned long aCount)
    {
        while (aCount--)
        {
            // make sure the compiler doesn't optimize out the decrementing so that this is
            // an actual cycle count delay
            asm volatile("nop");
        }
    }
}