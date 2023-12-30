#ifndef KERNEL_PERIPHERALS_BASE_H
#define KERNEL_PERIPHERALS_BASE_H

#include <cstdint>
#include "../MemoryManager.h"
#include "../PointerTypes.h"

namespace MemoryMappedIO
{
    // #TODO: Build custom type for memory mapped registers
    // At some point we're going to want a type for memory mapped registers instead of just integers for type safety
    // and ease of use. However we don't currently have support for static constructors/destructors (no C++ runtime)
    // so we likely need that first.

    // #TODO: Remove GetAddress when we fix the types
    
    /**
     * Base address for all memory-mapped peripherals - note that documentation will show addresses in the 0x7Exxxxxx
     * range (the "bus address"), so to figure out the value to use in the kernel (the "physical address") just trim
     * the top byte add the resulting value to this
     */
    constexpr VirtualPtr PeripheralBaseAddr = VirtualPtr{ MemoryManager::DeviceBaseAddress.GetAddress() }.Offset(MemoryManager::KernelVirtualAddressOffset);

    // Local peripheral information sourced from BCM2836 ARM-local peripheral documentation
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    constexpr VirtualPtr LocalPeripheralBaseAddr = VirtualPtr{ 0x4000'0000 }.Offset(MemoryManager::KernelVirtualAddressOffset);
}

#endif // KERNEL_PERIPHERALS_BASE_H