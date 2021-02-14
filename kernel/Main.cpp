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

    constexpr uint32_t GlobalTimerIntervalC = 200'000;
    constexpr uint32_t LocalTimerIntervalC = 9'600'000; // local timer seems to run faster, so higher interval for testing

    /**
     * Callback for timers
     * 
     * @param apParam The parameter given to RegisterCallback
     */
    void TimerCallback(const void* apParam)
    {
        Print::FormatToMiniUART("Timer callback: {}\r\n", static_cast<const char*>(apParam));
    }

    // Some hacky functions and classes for testing timers
    // #TODO: Figure out how frequency maps to timer interval
    // Having trouble figuring out the values to use for each type of timer. Just reading the clock
    // timer hz and using that directly does not equal a 1 second "tick" on either QEMU or real
    // hardware.
    // QEMU global timer: <QEMU does not implement>
    // QEMU local timer: ~8.5 seconds for 5 "ticks"
    // Hardware global timer: ~20 seconds for 1 "tick"
    // Hardware local timer: effectively instant (overflow?)
    // The online raspi3 tutorial github from bztsrc on github when using the global timer reads the
    // "hz" system register and the current counter, then adds ((hz/1000)*microsec)/1000 to the
    // current counter to time things in microseconds. But it does so using a while loop that reads
    // the current counter system register until it equals or matches the desired value, not using an
    // interrupt like we are.

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

    void CountdownCallback(const void* apParam)
    {
        const auto pconstCountdown = static_cast<const BaseCountdownData*>(apParam);
        const auto pcountdown = const_cast<BaseCountdownData*>(pconstCountdown); // UB unless we know the original is non-const (which we do)
        pcountdown->DecrementRemainingIntervals();
        Print::FormatToMiniUART("Countdown: {}\r\n", pcountdown->GetRemainingIntervals());
        if (pcountdown->GetRemainingIntervals() != 0)
        {
            pcountdown->RegisterCallback();
        }
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

        LocalCountdownData localTimerTest(1000); // #TODO: correct time on QEMU, far too fast on hardware
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
                // The raspi3 tutorials from bztsrc on github detect this by reading the high and
                // low memory mapped registers and seeing if they are 0. There might be a cleaner
                // or "more correct" way using the device tree, but we need to be able to read that
                // first
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