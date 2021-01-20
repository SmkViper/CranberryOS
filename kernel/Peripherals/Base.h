#ifndef KERNEL_PERIPHERALS_BASE_H
#define KERNEL_PERIPHERALS_BASE_H

#include <cstdint>

namespace MemoryMappedIO
{
    // TODO: At some point we're going to want a type for memory mapped registers instead of just integers for type
    // safety and ease of use. However we don't currently have support for static constructors/destructors (no C++
    // runtime) so we likely need that first.
    
    /**
     * Base address for all memory-mapped peripherals - note that documentation will show addresses in the 0x7Exxxxxx
     * range (the "bus address"), so to figure out the value to use in the kernel (the "physical address") just trim
     * the top byte add the resulting value to this
     */
    constexpr uintptr_t PeripheralBaseAddr = 0x3F00'0000;

    // Local peripheral information sourced from BCM2836 ARM-local peripheral documentation
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    constexpr uintptr_t LocalPeripheralBaseAddr = 0x4000'0000;
}

#endif // KERNEL_PERIPHERALS_BASE_H