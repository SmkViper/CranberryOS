#include "Timer.h"

#include <cstdint>
#include "Peripherals/Timer.h"
#include "Print.h"
#include "Utils.h"

namespace
{
    constexpr uint32_t IntervalC = 200'000;
    constexpr uint32_t LocalIntervalC = 9'600'000; // local timer seems to run faster, so higher interval for testing
    uint32_t CurTimerValue = 0;

    // Local timer documentation: https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    
    // Control register flags
    constexpr uint32_t LocalTimerControlEnableInterrupt = 1u << 29;
    constexpr uint32_t LocalTimerControlEnableTimer = 1u << 28;

    // Clear & reload flags
    constexpr uint32_t LocalTimerClearInterruptAck = 1u << 31;
}

namespace Timer
{
    // The global timer works by checking a global incrementing counter against each of four comparison registers. If
    // it matches one, then it triggers an interrupt. So basic operation of this timer is to set the compare register
    // to the value we want to trigger on, then update the comparison register when we get the interrupt.

    void Init()
    {
        CurTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        CurTimerValue += IntervalC;
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, CurTimerValue);
    }

    void HandleIRQ()
    {
        CurTimerValue += IntervalC;
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, CurTimerValue);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::ControlStatus, 1 << 1); // clearing the compare 1 signal
        Print::FormatToMiniUART("Timer interrupt received\r\n");
    }
}

namespace LocalTimer
{
    // The local timer counts its value down to 0, and triggers an interrupt when it resets at 0. We then have to clear
    // the interrupt bit so that the timer will re-trigger again.

    void Init()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 
            LocalIntervalC | LocalTimerControlEnableInterrupt | LocalTimerControlEnableTimer
        );
    }

    void HandleIRQ()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ClearAndReload, LocalTimerClearInterruptAck);
        Print::FormatToMiniUART("Local timer interrupt received\r\n");
    }
}