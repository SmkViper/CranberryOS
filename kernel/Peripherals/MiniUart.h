#ifndef KERNEL_PERIPHERALS_MINIUART_H
#define KERNEL_PERIPHERALS_MINIUART_H

#include "Base.h"

namespace MemoryMappedIO
{
    namespace Auxiliary
    {
        /** AUX_ENABLES - enables/disables the various auxiliary peripherals */
        constexpr uintptr_t EnablesRegister = (PeripheralBaseAddr + 0x00215004);
    }

    namespace MiniUART
    {
        // Note to use any of these registers, you need to enable the MiniUART by setting bit 0 on
        // Auxiliary::EnablesRegister first. GPIO pins should also be set up before enabling.

        /** AUX_MU_IO_REG - Used to read/write a byte from the MiniUART FIFO */
        constexpr uintptr_t IORegister =                (PeripheralBaseAddr + 0x00215040);
        /** AUX_MU_IER_REG - Used to enable/disable interrupts */
        constexpr uintptr_t InterruptEnableRegister =   (PeripheralBaseAddr + 0x00215044);
        /** AUX_MU_IIR_REG - Shows interrupt status and allows clearing FIFO */
        constexpr uintptr_t InterruptStatusRegister =   (PeripheralBaseAddr + 0x00215048);
        /** AUX_MU_LCR_REG - Controls line data format and can give access to the baudrate register */
        constexpr uintptr_t LineControlRegister =       (PeripheralBaseAddr + 0x0021504C);
        /** AUX_MU_MCR_REG - Controls the modem signals */
        constexpr uintptr_t ModemControlRegister =      (PeripheralBaseAddr + 0x00215050);
        /** AUX_MU_LSR_REG - Shows data status */
        constexpr uintptr_t LineStatusRegister =        (PeripheralBaseAddr + 0x00215054);
        /** AUX_MU_MSR_REG - Shows the moment status */
        constexpr uintptr_t ModemStatusRegister =       (PeripheralBaseAddr + 0x00215058);
        /** AUX_MU_SCRATCH - Single byte storage */
        constexpr uintptr_t ScratchRegister =           (PeripheralBaseAddr + 0x0021505C);
        /** AUX_MU_CNTL_REG - Accesses additional features */
        constexpr uintptr_t AdditionalControlRegister = (PeripheralBaseAddr + 0x00215060);
        /** AUX_MU_STAT_REG - Accesses additional status information */
        constexpr uintptr_t AdditionalStatusRegister =  (PeripheralBaseAddr + 0x00215064);
        /** AUX_MU_BAUD_REG - Direct access to the 16-bit wide baudrate counter */
        constexpr uintptr_t BaudRateRegister =          (PeripheralBaseAddr + 0x00215068);
    }
}

#endif // KERNEL_PERIPHERALS_MINIUART_H