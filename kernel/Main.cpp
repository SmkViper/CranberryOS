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

        // TODO
        // Tests currently crash on real hardware due to unaligned access (pointers to strings being dereferenced into
        // a 'x' register, which requires higher alignment than 1). -mno-unaligned-access does not seem to fix the
        // problem. The issue _should_ go away once we support virtual memory.
        // The tests work on QEMU, so this can be uncommented for QEMU runs to ensure certain things work.
        //UnitTests::Run();

        MiniUART::SendString("Hello, World!\r\n\tq = \"exit\" the kernel\r\n\tl = run a local timer\r\n\tg = run a global timer\r\n");

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
                LocalTimer::RegisterCallback(LocalTimerIntervalC, TimerCallback, "LOCAL");
                break;

            case 'g':
                // TODO
                // QEMU doesn't emulate this seems like, so it won't work there. Ideally we'd detect the timers available
                Timer::RegisterCallback(GlobalTimerIntervalC, TimerCallback, "GLOBAL");
                break;
            }
        }

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        CallStaticDestructors();

        UnitTests::RunPostStaticDestructors();
    }
}