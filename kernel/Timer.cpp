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

    void RegisterCallback(const uint32_t aIntervalMS, const CallbackFunctionPtr apCallback, const void* const apParam)
    {
        pGlobalTimerCallback = apCallback;
        pGlobalTimerParam = apParam;

        // The global timer (or "BCM system timer") runs at a fixed 1MHz frequency
        // Source: https://wiki.osdev.org/BCM_System_Timer
        // Experimentation with hardware shows this to be accurate
        //
        // #TODO: Figure out where this is specified, or if it can be (or needs to be) read at runtime from a device
        // tree or similar structure

        const auto intervalTicks = aIntervalMS * 1000;

        const auto curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + intervalTicks);
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

    void RegisterCallback(const uint32_t aIntervalMS, const CallbackFunctionPtr apCallback, const void* const apParam)
    {
        pLocalTimerCallback = apCallback;
        pLocalTimerParam = apParam;

        // OSDev wiki claims the timer operates at a 38.5MHz frequency, and testing in QEMU bears this out, as giving
        // it a countdown value of 38'500'000 results in a 1 second timer tick on QEMU. However on hardware it fires
        // almost instantly, so something is wrong either in the math or in how we're setting up the interrupt
        // Source: https://wiki.osdev.org/ARM_Local_Timer
        //
        // #TODO: Figure out why hardware is firing instantly instead of matching QEMU

        const auto intervalTicks = aIntervalMS * 38'400;

        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 
            intervalTicks | LocalTimerControlEnableInterrupt | LocalTimerControlEnableTimer
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