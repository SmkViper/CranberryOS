#ifndef KERNEL_PERIPHERALS_TIMER_H
#define KERNEL_PERIPHERALS_TIMER_H

#include "Base.h"

namespace MemoryMappedIO
{
    namespace Timer
    {
        // Status of each of the four timer interrupt lines - write 1 to clear the status to the bit
        constexpr uintptr_t ControlStatus =     PeripheralBaseAddr + 0x3000;
        // Current low 32 bits of the timer counter
        constexpr uintptr_t CounterLow =        PeripheralBaseAddr + 0x3004;
        // Current high 32 bits of the timer counter
        constexpr uintptr_t CounterHigh =       PeripheralBaseAddr + 0x3008;
        // Compare value for each of the four timer channels. When the low bits of the counter matches one of these,
        // the appropriate interrupt is signaled
        constexpr uintptr_t Compare0 =          PeripheralBaseAddr + 0x300C;
        constexpr uintptr_t Compare1 =          PeripheralBaseAddr + 0x3010;
        constexpr uintptr_t Compare2 =          PeripheralBaseAddr + 0x3014;
        constexpr uintptr_t Compare3 =          PeripheralBaseAddr + 0x3018;
    }

    namespace LocalTimer
    {
        // Local timer information sourced from BCM2836 ARM-local peripheral documentation
        // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf

        // The timer reload value, enable flags, and interrupt flag (read-only)
        constexpr uintptr_t ControlStatus =     LocalPeripheralBaseAddr + 0x0034;

        // Write-only flags for clearing the interrupt flag and telling it to reload
        constexpr uintptr_t ClearAndReload =    LocalPeripheralBaseAddr + 0x0038;
    }
}

#endif // KERNEL_PERIPHERALS_TIMER_H