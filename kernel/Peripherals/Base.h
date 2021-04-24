#ifndef KERNEL_PERIPHERALS_BASE_H
#define KERNEL_PERIPHERALS_BASE_H

#include <cstdint>
#include "../ARM/MMUDefines.h"
#include "../MemoryManager.h"

namespace MemoryMappedIO
{
    // #TODO: Build custom type for memory mapped registers
    // At some point we're going to want a type for memory mapped registers instead of just integers for type safety
    // and ease of use. However we don't currently have support for static constructors/destructors (no C++ runtime)
    // so we likely need that first.
    
    /**
     * Base address for all memory-mapped peripherals - note that documentation will show addresses in the 0x7Exxxxxx
     * range (the "bus address"), so to figure out the value to use in the kernel (the "physical address") just trim
     * the top byte add the resulting value to this
     */
    // #TODO: Fix up device addressing to take MMU into account
    // If we allow the base addr here to be relative to the virtual address start of the kernel then the linker starts
    // complaining about things being out of range, likely due to trying to read/write to a very high address from what
    // it thinks is a very low address (because the linker currently thinks the kernel loads at 0x80000 even though it
    // will be at the virtual address start once we turn the MMU on). Going to need more investigation.
    constexpr uintptr_t PeripheralBaseAddr = /*MemoryManager::KernalVirtualAddressStart +*/ DEVICE_BASE;

    // Local peripheral information sourced from BCM2836 ARM-local peripheral documentation
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    constexpr uintptr_t LocalPeripheralBaseAddr = 0x4000'0000;
}

#endif // KERNEL_PERIPHERALS_BASE_H