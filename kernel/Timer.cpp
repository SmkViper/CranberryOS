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
    constexpr uint32_t LocalTimerReload = 1u << 30;

    // Have to save this off so we can access it and set up the global timer to re-fire
    uint32_t GlobalTimerInterval = 0u;
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
        auto fireAgain = false;
        if (pGlobalTimerCallback != nullptr)
        {
            fireAgain = pGlobalTimerCallback(pGlobalTimerParam);
        }
        if (fireAgain)
        {
            const auto curTimerValue = MemoryMappedIO::Get32(MemoryMappedIO::Timer::CounterLow);
            MemoryMappedIO::Put32(MemoryMappedIO::Timer::Compare1, curTimerValue + GlobalTimerInterval);
        }
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

        // #TODO: Investigate QEMU timer register and speed and hardware instant trigger
        // OSDev wiki claims the timer operates at a 38.4MHz frequency, which matches the QA7 documentation specifying
        // that the clock here ticks on every crystal clock edge with the crystal clock running at 19.2MHz.
        // The real hardware reports 19.2MHz via the system counter clock frequency register, so the below code works
        // on real hardware (with one minor issue mentioned later). However QEMU reports the wrong system counter clock
        // frequency (a much faster 62.5MHz) which is strangely consistent. This results in the timer firing too slowly
        // in QEMU. QEMU itself seems to hard-code the interrupt at the expected 38.4MHz frequency however, so if we
        // want to bodge in a QEMU detection method and hard code the ticksPerMS, then we could get a consistent
        // counter for both.
        // Source: https://wiki.osdev.org/ARM_Local_Timer
        //
        // Additionaly, for some reason, the hardware will instantly fire off an interrupt after the control status
        // register is written, rather than waiting for the countdown. QEMU does not appear to have this issue, so more
        // investigation needs to be done as to why the hardware is behaving this way.

        // We get one clock tick on every edge, so tick frequency is twice that of the reported clock frequency
        const auto ticksPerSecond = (Timing::GetSystemCounterClockFrequencyHz() * 2u);
        const auto ticksPerMS = ticksPerSecond / 1000u;
        const auto intervalTicks = aIntervalMS * ticksPerMS;

        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 
            intervalTicks | LocalTimerControlEnableInterrupt | LocalTimerControlEnableTimer
        );
    }

    void HandleIRQ()
    {
        MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ClearAndReload, LocalTimerClearInterruptAck);
        auto fireAgain = false;
        if (pLocalTimerCallback != nullptr)
        {
            fireAgain = pLocalTimerCallback(pLocalTimerParam);
        }
        if (!fireAgain)
        {
            // writing zero clears the enable and interrupt flags, so we shouldn't trigger another callback
            MemoryMappedIO::Put32(MemoryMappedIO::LocalTimer::ControlStatus, 0);
        }
    }
}