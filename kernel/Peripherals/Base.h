#ifndef KERNEL_PERIPHERALS_BASE_H
#define KERNEL_PERIPHERALS_BASE_H

namespace MemoryMappedIO
{
    // TODO: At some point we're going to want a type for memory mapped registers instead of just unsigned longs for
    // type safety and ease of use. However we don't currently have support for static constructors/destructors (no
    // C++ runtime) so we likely need that first.
    
    // We don't have a standard library at this level, so for now, just make sure we don't make a mistake
    static_assert(sizeof(unsigned long) == sizeof(void*), "Unexpected pointer size");

    /**
     * Base address for all memory-mapped peripherals - note that documentation will show addresses in the 0x7Exxxxxx
     * range (the "bus address"), so to figure out the value to use in the kernel (the "physical address") just trim
     * the top byte add the resulting value to this
     */
    constexpr unsigned long PeripheralBaseAddr = 0x3F000000;
}

#endif // KERNEL_PERIPHERALS_BASE_H