#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include <cstdint>

namespace MemoryMappedIO
{
    /**
     * Put a 32 bit data value into the given address
     * 
     * @param aAddress Address to store data into
     * @param aData Data to store
     */
    void Put32(uintptr_t aAddress, uint32_t aData);

    /**
     * Obtain a 32 bit data value from the given address
     * 
     * @param aAddress Address to read data from
     * @return The value store there
     */
    uint32_t Get32(uintptr_t aAddress);
}

namespace Timing
{
    /**
     * Delay by busy-looping until the counter expires
     * 
     * @param aCount Value to count down to 0 (cycle count)
     */
    void Delay(uint64_t aCount);
}

namespace CPU
{
    /**
     * Obtain the current exception level the CPU is running at
     * 
     * @return The current exception level (0 is user land, 1 is typical OS, 2 is hypervisor, 3 is secure/insecure worlds)
     */
    uint64_t GetExceptionLevel();
}

#endif // KERNEL_UTILS_H