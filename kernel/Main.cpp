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

    constexpr const char* testArray[] = {
        "String 1",
        "String 2"
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

        // TODO
        // Tests currently crash on real hardware due to unaligned access (pointers to strings being dereferenced into
        // a 'x' register, which requires higher alignment than 1). -mno-unaligned-access does not seem to fix the
        // problem. The issue _should_ go away once we support virtual memory.
        // The tests work on QEMU, so this can be uncommented for QEMU runs to ensure certain things work.
        //UnitTests::Run();

        MiniUART::SendString("Hello, World!\r\n\tq = \"exit\" the kernel\r\n\tl = run a local timer\r\n\tg = run a global timer\r\n");

        // TODO
        // Figure out how to stop the timers
        
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
                LocalTimer::Init();
                break;

            case 'g':
                // TODO
                // QEMU doesn't emulate this seems like, so it won't work there. Ideally we'd detect the timers available
                Timer::Init();
                break;
            }
        }

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        CallStaticDestructors();

        UnitTests::RunPostStaticDestructors();
    }
}