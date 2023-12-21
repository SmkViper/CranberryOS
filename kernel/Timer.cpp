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
    // constexpr uint32_t LocalTimerReload = 1u << 30; // currently unused

    // Have to save this off so we can access it and set up the global timer to re-fire
    uint32_t GlobalTimerInterval = 0u;

    /**
     * "Sanitizes" the frequency reported by the system counter clock so it can be used to set up the local timer
     * 
     * @param aFrequency The frequency to sanitize
     * @return The sanitized frequency
     */
    uint64_t SanitizeLocalTimerFrequency(const uint64_t aFrequency)
    {
        auto retVal = aFrequency;
        // QEMU seems to not report the correct clock frequency, at least in respect to trying to use this frequency to
        // set up a local timer. So to get the local timer working for both QEMU and real hardware, we're going to
        // "detect" the presence of QEMU by seeing if the reported frequency here is higher than we expect. If it is,
        // fake the clock frequency with a hardcoded value that matches QEMU's local timer speed
        //
        // #TODO: Figure out if there's a better way to handle this
        if (aFrequency > 50'000'000)
        {
            Print::FormatToMiniUART("[\x1b[33mWARN\x1b[m] Excessive clock frequency {}Hz, faking hard-coded clock\n", aFrequency);
            // fake 19.2MHz crystal clock
            retVal = 19'200'000;
        }
        return retVal;
    }
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

        GlobalTimerInterval = aIntervalMS * 1000;

        const auto curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + GlobalTimerInterval);
    }

    void HandleIRQ()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::ControlStatus, 1 << 1); // clearing the compare 1 signal

        // set up the timer to trigger again
        const auto curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + GlobalTimerInterval);
        
        pGlobalTimerCallback(pGlobalTimerParam);
    }
}

namespace LocalTimer
{
    // The local timer counts its value down to 0, and triggers an interrupt when it reaches 0 before reloading the
    // counter value and counting down again. So once we set the trigger value, it will trigger periodically until
    // disabled.

    void RegisterCallback(const uint32_t aIntervalMS, const CallbackFunctionPtr apCallback, const void* const apParam)
    {
        pLocalTimerCallback = apCallback;
        pLocalTimerParam = apParam;

        // #TODO: Investigate hardware instant trigger
        // For some reason, the hardware will instantly fire off an interrupt after the control status register is
        // written, rather than waiting for the countdown. QEMU does not appear to have this behavior, so more
        // investigation needs to be done as to why the hardware is behaving this way.

        // This timer ticks on every crystal clock edge - which is why we double the clock frequency to find the number
        // of ticks per second. (See QA7 documentation, section 4.11)

        const auto ticksPerSecond = (SanitizeLocalTimerFrequency(Timing::GetSystemCounterClockFrequencyHz()) * 2u);
        const auto ticksPerMS = ticksPerSecond / 1000u;
        const auto intervalTicks = static_cast<uint32_t>(aIntervalMS * ticksPerMS);

        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 
            intervalTicks | LocalTimerControlEnableInterrupt | LocalTimerControlEnableTimer
        );
    }

    void HandleIRQ()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ClearAndReload, LocalTimerClearInterruptAck);
        
        pLocalTimerCallback(pLocalTimerParam);
    }
}