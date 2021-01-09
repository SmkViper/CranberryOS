#ifndef KERNEL_PERIPHERALS_IRQ_H
#define KERNEL_PERIPHERALS_IRQ_H

#include "Base.h"

namespace MemoryMappedIO
{
    namespace IRQ
    {
        // Shows which basic interrupts are pending
        constexpr uintptr_t IRQBasicPending =       PeripheralBaseAddr + 0xB200;
        // Shows which interrupts 0-31 are pending
        constexpr uintptr_t IRQPending1 =           PeripheralBaseAddr + 0xB204;
        // Shows which interrupts 32-63 are pending
        constexpr uintptr_t IRQPending2 =           PeripheralBaseAddr + 0xB208;
        // Selects which interrupt source can generate a FIQ
        constexpr uintptr_t FIQSource =             PeripheralBaseAddr + 0xB20C;
        // Enables IRQ sources 0-31
        constexpr uintptr_t InterruptEnable1 =      PeripheralBaseAddr + 0xB210;
        // Enables IRQ sources 32-63
        constexpr uintptr_t InterruptEnable2 =      PeripheralBaseAddr + 0xB214;
        // Enables basic interrupts
        constexpr uintptr_t BasicInterruptEnable =  PeripheralBaseAddr + 0xB218;
        // Disables IRQ sources 0-31
        constexpr uintptr_t InterruptDisable1 =     PeripheralBaseAddr + 0xB21C;
        // Disables IRQ sources 32-63
        constexpr uintptr_t InterruptDisable2 =     PeripheralBaseAddr + 0xB220;
        // Disables basic interrupts
        constexpr uintptr_t BasicInterruptDisable = PeripheralBaseAddr + 0xB224;
    }
}

#endif // KERNEL_PERIPHERALS_IRQ_H