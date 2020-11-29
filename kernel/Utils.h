#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

namespace MemoryMappedIO
{
    // We don't have a standard library at this level, so for now, just make sure we don't make a mistake
    static_assert(sizeof(unsigned long) == sizeof(void*), "Unexpected pointer size");

    /**
     * Put a 32 bit data value into the given address
     * 
     * @param aAddress Address to store data into
     * @param aData Data to store
     */
    void Put32(unsigned long aAddress, unsigned int aData);

    /**
     * Obtain a 32 bit data value from the given address
     * 
     * @param aAddress Address to read data from
     * @return The value store there
     */
    unsigned int Get32(unsigned long aAddress);
}

namespace Timing
{
    /**
     * Delay by busy-looping until the counter expires
     * 
     * @param aCount Value to count down to 0 (cycle count)
     */
    void Delay(unsigned long aCount);
}

#endif // KERNEL_UTILS_H