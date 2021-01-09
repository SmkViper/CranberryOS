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
    };

    // Timer IRQ0 is reserved by GPU
    static constexpr uint32_t SystemTimerIRQ1 = 1 << 1;
    // Timer IRQ2 is reserved by GPU
    static constexpr uint32_t SystemTimerIRQ3 = 1 << 3;
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
    void show_invalid_entry_message(int32_t aType, uint64_t aESR, uint64_t aReturnAddress)
    {
        // TODO
        // Format ESR and address as hex once we have support for that
        Print::FormatToMiniUART("{}, ESR: {}, address: {}\r\n", ExceptionTypeNames[aType], aESR, aReturnAddress);
    }

    /**
     * Handles an IRQ triggering
     */
    void handle_irq()
    {
        const auto irqPending1 = MemoryMappedIO::Get32(MemoryMappedIO::IRQ::IRQPending1);

        // TODO
        // Multiple flags can be set at the same time, so we'll want to handle them all
        switch (irqPending1)
        {
        case SystemTimerIRQ1:
            Timer::HandleIRQ();
            break;

        default:
            // TODO
            // Format pending as hex once we have support for that
            Print::FormatToMiniUART("Unknown pending IRQ: {}\r\n", irqPending1);
            break;
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