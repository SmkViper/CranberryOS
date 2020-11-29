#ifndef KERNEL_PERIPHERALS_GPIO_H
#define KERNEL_PERIPHERALS_GPIO_H

#include "Base.h"

namespace MemoryMappedIO
{
    namespace GPIO
    {
        /** GPFSEL1 - selects the operation of specific GPIO pins. For pins 10-19 */
        constexpr unsigned long FunctionSelect1Register =   (PeripheralBaseAddr + 0x00200004);
        /** GPSET0 - sets a specific GPIO pin, for output pins. For pins 0-31 */
        constexpr unsigned long PinOutputSet0Register =     (PeripheralBaseAddr + 0x0020001C);
        /** GPCLR0 - clears a specific GPIO pin, for output pins. For pins 0-31 */
        constexpr unsigned long PinOutputClear0Register =   (PeripheralBaseAddr + 0x00200028);
        /**
         * GPPUD - controls actuation of internal pull up/down line for all pins (use with
         * PullUpDownClockRegister to effect changes)
         */
        constexpr unsigned long PullUpDownRegister =        (PeripheralBaseAddr + 0x00200094);
        /**
         * GPPUDCLK0 - controls actuation of internal pull-downs on respective pins (use with
         * PullUpDownRegister to effect changes)
         */
        constexpr unsigned long PullUpDownClock0Register =  (PeripheralBaseAddr + 0x00200098);
    }
}

#endif // KERNEL_PERIPHERALS_GPIO_H