#include "Timer.h"

#include <cstdint>
#include "Peripherals/Timer.h"
#include "Print.h"
#include "Utils.h"

namespace
{
    // #TODO: Likely a better way we can do this, and once we figure that out, the lint tag can be removed
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

    Timer::CallbackFunctionPtr pGlobalTimerCallback = nullptr;
    const void* pGlobalTimerParam = nullptr;
    LocalTimer::CallbackFunctionPtr pLocalTimerCallback = nullptr;
    const void* pLocalTimerParam = nullptr;

    // Local timer documentation: https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
    
    // Control register flags
    constexpr uint32_t LocalTimerControlEnableInterrupt = 1U << 29U;
    constexpr uint32_t LocalTimerControlEnableTimer = 1U << 28U;

    // Clear & reload flags
    constexpr uint32_t LocalTimerClearInterruptAck = 1U << 31U;
    // constexpr uint32_t LocalTimerReload = 1U << 30U; // currently unused

    // Have to save this off so we can access it and set up the global timer to re-fire
    uint32_t GlobalTimerInterval = 0U;

    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    /**
     * "Sanitizes" the frequency reported by the system counter clock so it can be used to set up the local timer
     * 
     * @param aFrequency The frequency to sanitize
     * @return The sanitized frequency
     */
    uint64_t SanitizeLocalTimerFrequency(uint64_t const aFrequency)
    {
        auto retVal = aFrequency;
        // QEMU seems to not report the correct clock frequency, at least in respect to trying to use this frequency to
        // set up a local timer. So to get the local timer working for both QEMU and real hardware, we're going to
        // "detect" the presence of QEMU by seeing if the reported frequency here is higher than we expect. If it is,
        // fake the clock frequency with a hardcoded value that matches QEMU's local timer speed
        //
        // #TODO: Figure out if there's a better way to handle this
        constexpr uint64_t MaxFrequency = 50'000'000U;
        constexpr uint64_t FakeFrequency = 19'200'000U; // fake 19.2MHz crystal clock
        if (aFrequency > MaxFrequency)
        {
            Print::FormatToMiniUART("[\x1b[33mWARN\x1b[m] Excessive clock frequency {}Hz, faking hard-coded clock\n", aFrequency);
            retVal = FakeFrequency;
        }
        return retVal;
    }
}

namespace Timer
{
    // The global timer works by checking a global incrementing counter against each of four comparison registers. If
    // it matches one, then it triggers an interrupt. So basic operation of this timer is to set the compare register
    // to the value we want to trigger on, then update the comparison register when we get the interrupt.

    void RegisterCallback(uint32_t const aIntervalMS, CallbackFunctionPtr const apCallback, void const* const apParam)
    {
        pGlobalTimerCallback = apCallback;
        pGlobalTimerParam = apParam;

        // The global timer (or "BCM system timer") runs at a fixed 1MHz frequency
        // Source: https://wiki.osdev.org/BCM_System_Timer
        // Experimentation with hardware shows this to be accurate
        //
        // #TODO: Figure out where this is specified, or if it can be (or needs to be) read at runtime from a device
        // tree or similar structure
        constexpr uint32_t MSToTimerInterval = 1'000;
        GlobalTimerInterval = aIntervalMS * MSToTimerInterval;

        auto const curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + GlobalTimerInterval);
    }

    void HandleIRQ()
    {
        constexpr uint32_t TimerMatch1Bit = 0x1U << 1U;
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::ControlStatus, TimerMatch1Bit); // clearing the compare 1 signal

        // set up the timer to trigger again
        auto const curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
        MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + GlobalTimerInterval);
        
        pGlobalTimerCallback(pGlobalTimerParam);
    }
}

namespace LocalTimer
{
    // The local timer counts its value down to 0, and triggers an interrupt when it reaches 0 before reloading the
    // counter value and counting down again. So once we set the trigger value, it will trigger periodically until
    // disabled.

    void RegisterCallback(uint32_t const aIntervalMS, CallbackFunctionPtr const apCallback, void const* const apParam)
    {
        pLocalTimerCallback = apCallback;
        pLocalTimerParam = apParam;

        // #TODO: Investigate hardware instant trigger
        // For some reason, the hardware will instantly fire off an interrupt after the control status register is
        // written, rather than waiting for the countdown. QEMU does not appear to have this behavior, so more
        // investigation needs to be done as to why the hardware is behaving this way.

        // This timer ticks on every crystal clock edge - which is why we double the clock frequency to find the number
        // of ticks per second. (See QA7 documentation, section 4.11)
        constexpr uint32_t frequencyToTicksPerSecond = 2U;

        const auto ticksPerSecond = (SanitizeLocalTimerFrequency(Timing::GetSystemCounterClockFrequencyHz()) * frequencyToTicksPerSecond);
        const auto ticksPerMS = ticksPerSecond / 1000U;
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