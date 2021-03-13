#include <cstdint>
#include "Peripherals/IRQ.h"
#include "Print.h"
#include "Timer.h"
#include "Utils.h"

namespace
{
    // Order should match #define values in ARM/ExceptionVectorDefines.h
    constexpr const char* ExceptionTypeNames[] = {
        "SYNC_INVALID_EL1t",
        "IRQ_INVALID_EL1t",
        "FIQ_INVALID_EL1t",
        "ERROR_INVALID_EL1t",

        "SYNC_INVALID_EL1h",
        "IRQ_INVALID_EL1h",
        "FIQ_INVALID_EL1h",
        "ERROR_INVALID_EL1h",

        "SYNC_INVALID_EL0_64",
        "IRQ_INVALID_EL0_64",
        "FIQ_INVALID_EL0_64",
        "ERROR_INVALID_EL0_64",

        "SYNC_INVALID_EL0_32",
        "IRQ_INVALID_EL0_32",
        "FIQ_INVALID_EL0_32",
        "ERROR_INVALID_EL0_32",

        "SYNC_ERROR",
        "SYSCALL_ERROR",
    };

    // Timer IRQ0 is reserved by GPU
    static constexpr uint32_t SystemTimerIRQ1 = 1 << 1;
    // Timer IRQ2 is reserved by GPU
    static constexpr uint32_t SystemTimerIRQ3 = 1 << 3;

    // Sourced from:
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    static constexpr uint32_t LocalTimerIRQ = 1u << 11;
}

extern "C"
{
    /**
     * Spits out an error message for an exception type we don't currently handle
     * 
     * @param aType Exception type index (see ARM/ExceptionVectorDefines.h)
     * @param aESR The contents of the ESR register
     * @param aReturnAddress The address the exception occurred at
     */
    void show_invalid_entry_message(int64_t aType, uint64_t aESR, uint64_t aReturnAddress)
    {
        Print::FormatToMiniUART("{}:\r\n\tESR: {:x}\r\n\tAddress: {:x}\r\n", ExceptionTypeNames[aType], aESR, aReturnAddress);
    }

    /**
     * Handles an IRQ triggering
     */
    void handle_irq()
    {
        // TODO
        // Multiple flags can be set at the same time, so we'll want to handle them all

        const auto irqPending1 = MemoryMappedIO::Get32(MemoryMappedIO::IRQ::IRQPending1);
        if (irqPending1 != 0)
        {
            switch (irqPending1)
            {
            case SystemTimerIRQ1:
                Timer::HandleIRQ();
                break;

            default:
                Print::FormatToMiniUART("Unknown pending IRQ: {:x}\r\n", irqPending1);
                break;
            }
        }

        const auto core0IRQPending = MemoryMappedIO::Get32(MemoryMappedIO::IRQ::Core0IRQSource);
        if (core0IRQPending != 0)
        {
            switch (core0IRQPending)
            {
            case LocalTimerIRQ:
                LocalTimer::HandleIRQ();
                break;

            default:
                Print::FormatToMiniUART("Unknown pending Core 0 IRQ: {:x}\r\n", core0IRQPending);
                break;
            }
        }
    }
}

namespace ExceptionVectors
{
    void EnableInterruptController()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::IRQ::InterruptEnable1, SystemTimerIRQ1);
    }
}