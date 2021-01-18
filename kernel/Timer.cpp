#include "Timer.h"

#include <cstdint>
#include "Peripherals/Timer.h"
#include "Print.h"
#include "Utils.h"

namespace
{
    constexpr uint32_t IntervalC = 200000;
    uint32_t CurTimerValue = 0;
}

namespace Timer
{
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