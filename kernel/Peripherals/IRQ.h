#ifndef KERNEL_PERIPHERALS_IRQ_H
#define KERNEL_PERIPHERALS_IRQ_H

#include "Base.h"

namespace MemoryMappedIO::IRQ
{
    // Shows which basic interrupts are pending
    constexpr VirtualPtr IRQBasicPending =      PeripheralBaseAddr.Offset(0xB200);
    // Shows which interrupts 0-31 are pending
    constexpr VirtualPtr IRQPending1 =          PeripheralBaseAddr.Offset(0xB204);
    // Shows which interrupts 32-63 are pending
    constexpr VirtualPtr IRQPending2 =          PeripheralBaseAddr.Offset(0xB208);
    // Selects which interrupt source can generate a FIQ
    constexpr VirtualPtr FIQSource =            PeripheralBaseAddr.Offset(0xB20C);
    // Enables IRQ sources 0-31
    constexpr VirtualPtr InterruptEnable1 =     PeripheralBaseAddr.Offset(0xB210);
    // Enables IRQ sources 32-63
    constexpr VirtualPtr InterruptEnable2 =     PeripheralBaseAddr.Offset(0xB214);
    // Enables basic interrupts
    constexpr VirtualPtr BasicInterruptEnable = PeripheralBaseAddr.Offset(0xB218);
    // Disables IRQ sources 0-31
    constexpr VirtualPtr InterruptDisable1 =    PeripheralBaseAddr.Offset(0xB21C);
    // Disables IRQ sources 32-63
    constexpr VirtualPtr InterruptDisable2 =    PeripheralBaseAddr.Offset(0xB220);
    // Disables basic interrupts
    constexpr VirtualPtr BasicInterruptDisable = PeripheralBaseAddr.Offset(0xB224);

    // Below sourced from:
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf

    // Shows which interrupts are pending for Core0
    constexpr VirtualPtr Core0IRQSource =       LocalPeripheralBaseAddr.Offset(0x0060);
}

#endif // KERNEL_PERIPHERALS_IRQ_H