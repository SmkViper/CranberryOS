#include "ExceptionVectorHandlers.h"
#include "IRQ.h"
#include "MiniUart.h"
#include "Print.h"
#include "Timer.h"
#include "UnitTests.h"
#include "Utils.h"

namespace
{
    using StaticInitFunction = void (*)();
    using StaticFiniFunction = void (*)();
}

extern "C"
{
    // Defined by the linker to point at the start/end of the init/fini arrays
    extern StaticInitFunction __init_start;
    extern StaticInitFunction __init_end;
    extern StaticFiniFunction __fini_start;
    extern StaticFiniFunction __fini_end;
}

namespace
{
    /**
     * Calls all static constructors in our .init_array section
     */
    void CallStaticConstructors()
    {
        for (auto pcurFunc = &__init_start; pcurFunc != &__init_end; ++pcurFunc)
        {
            (*pcurFunc)();
        }
    }

    /**
     * Calls all static destructors in our .fini_array section
     */
    void CallStaticDestructors()
    {
        for (auto pcurFunc = &__fini_start; pcurFunc != &__fini_end; ++pcurFunc)
        {
            (*pcurFunc)();
        }
    }
    
    // #TODO: Cleanup and remove once we get something else using timers for testing and the like
    // Some hacky functions and classes for testing timers
    // QEMU global timer: <QEMU does not implement>

    class BaseCountdownData
    {
    public:
        void ResetRemainingIntervals(uint32_t aRemainingIntervals) { RemainingIntervals = aRemainingIntervals; }
        uint32_t GetRemainingIntervals() const { return RemainingIntervals; }
        void DecrementRemainingIntervals() { --RemainingIntervals; }

        virtual void RegisterCallback() = 0;

    private:
        uint32_t RemainingIntervals = 0;
    };

    bool CountdownCallback(const void* apParam)
    {
        const auto pconstCountdown = static_cast<const BaseCountdownData*>(apParam);
        const auto pcountdown = const_cast<BaseCountdownData*>(pconstCountdown); // UB unless we know the original is non-const (which we do)
        pcountdown->DecrementRemainingIntervals();
        Print::FormatToMiniUART("Countdown: {}\r\n", pcountdown->GetRemainingIntervals());
        return pcountdown->GetRemainingIntervals() != 0;
    }

    class LocalCountdownData: public BaseCountdownData
    {
    public:
        explicit LocalCountdownData(uint32_t aIntervalDuration) : IntervalDuration{aIntervalDuration} {}
        void RegisterCallback() override
        {
            LocalTimer::RegisterCallback(IntervalDuration, CountdownCallback, static_cast<BaseCountdownData*>(this));
        }

    private:
        uint32_t IntervalDuration = 0u;
    };

    class GlobalCountdownData: public BaseCountdownData
    {
    public:
        explicit GlobalCountdownData(uint32_t aIntervalDuration) : IntervalDuration{aIntervalDuration} {}
        void RegisterCallback() override
        {
            Timer::RegisterCallback(IntervalDuration, CountdownCallback, static_cast<BaseCountdownData*>(this));
        }

    private:
        uint32_t IntervalDuration = 0u;
    };
}

// Called from assembly, so don't mangle the name
extern "C"
{
    /**
     * Kernel entry point
     */
    void kmain()
    {
        CallStaticConstructors();

        MiniUART::Init();
        irq_vector_init();
        ExceptionVectors::EnableInterruptController();
        enable_irq();

        // #TODO: Fix crashing tests by implementing MMU support
        // Tests currently crash on real hardware due to unaligned access (pointers to strings being dereferenced into
        // a 'x' register, which requires higher alignment than 1). -mno-unaligned-access does not seem to fix the
        // problem. The issue _should_ go away once we support virtual memory.
        // The tests work on QEMU, so this can be uncommented for QEMU runs to ensure certain things work.
        //UnitTests::Run();

        const auto clockFrequencyHz = Timing::GetSystemCounterClockFrequencyHz();
        Print::FormatToMiniUART("System clock freq: {}hz\r\n", clockFrequencyHz);

        MiniUART::SendString("Hello, World!\r\n\tq = \"exit\" the kernel\r\n\tl = run a local timer\r\n\tg = run a global timer\r\n");

        LocalCountdownData localTimerTest(1000); // #TODO: first callback fires immediately on hardware
        GlobalCountdownData globalTimerTest(1000); // #TODO: correct time on hardware, not emulated on QEMU

        bool done = false;
        while(!done)
        {
            const auto valueEntered = MiniUART::Receive();
            MiniUART::Send(valueEntered); // always echo it to the user
            switch (valueEntered)
            {
            case 'q':
                done = true;
                break;

            case 'l':
                localTimerTest.ResetRemainingIntervals(5);
                localTimerTest.RegisterCallback();
                break;

            case 'g':
                // #TODO: Detect presence of QEMU and/or lack of global timer.
                // The raspi3 tutorials from bztsrc on github detect this by reading the high and low memory mapped
                // registers and seeing if they are 0. Experimentation shows that at least the current version of
                // QEMU reports semi-sane values, dispite the global timer (or at least the interrupts) being non-
                // operative. Might be able to detect the presence/absence of the timer via device tree parsing.
                globalTimerTest.ResetRemainingIntervals(5);
                globalTimerTest.RegisterCallback();
                break;
            }
        }

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        CallStaticDestructors();

        UnitTests::RunPostStaticDestructors();
    }
}