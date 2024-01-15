#include <cstdint>
#include "Peripherals/IRQ.h"
#include "PointerTypes.h"
#include "Print.h"
#include "Timer.h"
#include "Utils.h"

namespace
{
    // Order should match #define values in AArch64/ExceptionVectorDefines.h
    // #TODO: Can remove lint disable when std::array available
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    constexpr char const* const ExceptionTypeNames[] = {
        "SYNC_INVALID_EL1t",
        "IRQ_INVALID_EL1t",
        "FIQ_INVALID_EL1t",
        "ERROR_INVALID_EL1t",

        "SYNC_INVALID_EL1h",
        "FIQ_INVALID_EL1h",
        "ERROR_INVALID_EL1h",

        "FIQ_INVALID_EL0_64",
        "ERROR_INVALID_EL0_64",

        "SYNC_INVALID_EL0_32",
        "IRQ_INVALID_EL0_32",
        "FIQ_INVALID_EL0_32",
        "ERROR_INVALID_EL0_32",

        "SYNC_ERROR",
        "SYSCALL_ERROR",
        "DATA_ABORT_ERROR"
    };

    // #TODO: I think this timer value is shared with timer code - can we eliminate the redundancy?
    // Timer IRQ0 is reserved by GPU
    constexpr uint32_t SystemTimerIRQ1 = 1U << 1U;
    // Timer IRQ2 is reserved by GPU
    // constexpr uint32_t SystemTimerIRQ3 = 1U << 3U;

    // Sourced from:
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    // constexpr uint32_t LocalTimerIRQ = 1U << 11U;
}

extern "C"
{
    // #TODO: Called from assembly, so no great way to eliminate the lint tag that I know of at this point
    /**
     * Spits out an error message for an exception type we don't currently handle
     * 
     * @param aType Exception type index (see AArch64/ExceptionVectorDefines.h)
     * @param aESR The contents of the ESR register
     * @param aReturnAddress The address the exception occurred at
     */
    void show_invalid_entry_message(int64_t aType, uint64_t aESR, uint64_t aReturnAddress) // NOLINT(bugprone-easily-swappable-parameters)
    {
        // #TODO: Can likely remove lint tag when we switch to std::array
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        auto const* const pexceptionTypeName = ExceptionTypeNames[aType];
        Print::FormatToMiniUART("{}:\r\n\tESR: {:x}\r\n\tAddress: {:x}\r\n", pexceptionTypeName, aESR, VirtualPtr{ aReturnAddress });
    }

    /**
     * Handles an IRQ triggering
     */
    void handle_irq()
    {
        // #TODO: Multiple flags can be set at the same time, so we'll want to handle them all

        const auto irqPending1 = MemoryMappedIO::Get32(MemoryMappedIO::IRQ::IRQPending1);
        if (irqPending1 != 0)
        {
            if (irqPending1 == SystemTimerIRQ1)
            {
                Timer::HandleIRQ();
            }
            else
            {
                Print::FormatToMiniUART("Unknown pending IRQ: {:x}\r\n", irqPending1);
            }
        }
        /* #TODO: Local core IRQs cannot be used until we figure out how to map in the local peripheral addresses
        const auto core0IRQPending = MemoryMappedIO::Get32(MemoryMappedIO::IRQ::Core0IRQSource);
        if (core0IRQPending != 0)
        {
            if (core0IRQPending == LocalTimerIRQ)
            {
                LocalTimer::HandleIRQ();
            }
            else
            {
                Print::FormatToMiniUART("Unknown pending Core 0 IRQ: {:x}\r\n", core0IRQPending);
            }
        }
        */
    }
}

namespace ExceptionVectors
{
    void EnableInterruptController()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::IRQ::InterruptEnable1, SystemTimerIRQ1);
    }
}