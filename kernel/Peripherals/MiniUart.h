#ifndef KERNEL_PERIPHERALS_MINIUART_H
#define KERNEL_PERIPHERALS_MINIUART_H

#include "Base.h"

namespace MemoryMappedIO
{
    namespace Auxiliary
    {
        /** AUX_ENABLES - enables/disables the various auxiliary peripherals */
        constexpr VirtualPtr EnablesRegister = PeripheralBaseAddr.Offset(0x0021'5004);
    }

    namespace MiniUART
    {
        // Note to use any of these registers, you need to enable the MiniUART by setting bit 0 on
        // Auxiliary::EnablesRegister first. GPIO pins should also be set up before enabling.

        /** AUX_MU_IO_REG - Used to read/write a byte from the MiniUART FIFO */
        constexpr VirtualPtr IORegister =               PeripheralBaseAddr.Offset(0x0021'5040);
        /** AUX_MU_IER_REG - Used to enable/disable interrupts */
        constexpr VirtualPtr InterruptEnableRegister =  PeripheralBaseAddr.Offset(0x0021'5044);
        /** AUX_MU_IIR_REG - Shows interrupt status and allows clearing FIFO */
        constexpr VirtualPtr InterruptStatusRegister =  PeripheralBaseAddr.Offset(0x0021'5048);
        /** AUX_MU_LCR_REG - Controls line data format and can give access to the baudrate register */
        constexpr VirtualPtr LineControlRegister =      PeripheralBaseAddr.Offset(0x0021'504C);
        /** AUX_MU_MCR_REG - Controls the modem signals */
        constexpr VirtualPtr ModemControlRegister =     PeripheralBaseAddr.Offset(0x0021'5050);
        /** AUX_MU_LSR_REG - Shows data status */
        constexpr VirtualPtr LineStatusRegister =       PeripheralBaseAddr.Offset(0x0021'5054);
        /** AUX_MU_MSR_REG - Shows the moment status */
        constexpr VirtualPtr ModemStatusRegister =      PeripheralBaseAddr.Offset(0x0021'5058);
        /** AUX_MU_SCRATCH - Single byte storage */
        constexpr VirtualPtr ScratchRegister =          PeripheralBaseAddr.Offset(0x0021'505C);
        /** AUX_MU_CNTL_REG - Accesses additional features */
        constexpr VirtualPtr AdditionalControlRegister = PeripheralBaseAddr.Offset(0x0021'5060);
        /** AUX_MU_STAT_REG - Accesses additional status information */
        constexpr VirtualPtr AdditionalStatusRegister = PeripheralBaseAddr.Offset(0x0021'5064);
        /** AUX_MU_BAUD_REG - Direct access to the 16-bit wide baudrate counter */
        constexpr VirtualPtr BaudRateRegister =         PeripheralBaseAddr.Offset(0x0021'5068);
    }
}

#endif // KERNEL_PERIPHERALS_MINIUART_H