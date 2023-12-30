#ifndef KERNEL_PERIPHERALS_GPIO_H
#define KERNEL_PERIPHERALS_GPIO_H

#include "Base.h"

namespace MemoryMappedIO
{
    namespace GPIO
    {
        /** GPFSEL1 - selects the operation of specific GPIO pins. For pins 10-19 */
        constexpr VirtualPtr FunctionSelect1Register =  PeripheralBaseAddr.Offset(0x0020'0004);
        /** GPSET0 - sets a specific GPIO pin, for output pins. For pins 0-31 */
        constexpr VirtualPtr PinOutputSet0Register =    PeripheralBaseAddr.Offset(0x0020'001C);
        /** GPCLR0 - clears a specific GPIO pin, for output pins. For pins 0-31 */
        constexpr VirtualPtr PinOutputClear0Register =  PeripheralBaseAddr.Offset(0x0020'0028);
        /**
         * GPPUD - controls actuation of internal pull up/down line for all pins (use with
         * PullUpDownClockRegister to effect changes)
         */
        constexpr VirtualPtr PullUpDownRegister =       PeripheralBaseAddr.Offset(0x0020'0094);
        /**
         * GPPUDCLK0 - controls actuation of internal pull-downs on respective pins (use with
         * PullUpDownRegister to effect changes)
         */
        constexpr VirtualPtr PullUpDownClock0Register = PeripheralBaseAddr.Offset(0x0020'0098);
    }
}

#endif // KERNEL_PERIPHERALS_GPIO_H