#include "Timer.h"

#include <cstdint>
#include "Peripherals/Timer.h"
#include "Print.h"
#include "Utils.h"

namespace
{
    Timer::CallbackFunctionPtr pGlobalTimerCallback = nullptr;
    const void* pGlobalTimerParam = nullptr;
    LocalTimer::CallbackFunctionPtr pLocalTimerCallback = nullptr;
    const void* pLocalTimerParam = nullptr;

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

    void RegisterCallback(const uint32_t aInterval, const CallbackFunctionPtr apCallback, const void* const apParam)
    {
        pGlobalTimerCallback = apCallback;
        pGlobalTimerParam = apParam;

        const auto curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + aInterval);
    }

    void HandleIRQ()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, 0u); // make sure we don't trigger again
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::ControlStatus, 1 << 1); // clearing the compare 1 signal
        if (pGlobalTimerCallback != nullptr)
        {
            pGlobalTimerCallback(pGlobalTimerParam);
        }
    }
}

namespace LocalTimer
{
    // The local timer counts its value down to 0, and triggers an interrupt when it resets at 0. We then have to clear
    // the interrupt bit so that the timer will re-trigger again.

    void RegisterCallback(const uint32_t aInterval, const CallbackFunctionPtr apCallback, const void* const apParam)
    {
        pLocalTimerCallback = apCallback;
        pLocalTimerParam = apParam;

        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 
            aInterval | LocalTimerControlEnableInterrupt | LocalTimerControlEnableTimer
        );
    }

    void HandleIRQ()
    {
        // writing zero clears the enable and interrupt flags, so we shouldn't trigger another callback
        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 0);

        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ClearAndReload, LocalTimerClearInterruptAck);
        
        if (pLocalTimerCallback != nullptr)
        {
            pLocalTimerCallback(pLocalTimerParam);
        }
    }
}