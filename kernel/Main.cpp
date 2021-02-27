#include "ExceptionVectorHandlers.h"
#include "IRQ.h"
#include "MiniUart.h"
#include "Print.h"
#include "Scheduler.h"
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
    
    /**
     * A simple "process" to illustate that the task scheduler is working
     * 
     * @param apArray The string to output, one character at a time
     */
    void Process(const void* const apArray)
    {
        const auto apString = reinterpret_cast<const char*>(apArray);
        while (true)
        {
            for (auto curChar = 0; apString[curChar] != '\0'; ++curChar)
            {
                MiniUART::Send(apString[curChar]);
                Timing::Delay(100000);
            }
        }
        // Don't ever return, scheduler isn't expecting it
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
        Scheduler::InitTimer();
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

        MiniUART::SendString("Hello, World!\r\n");

        auto processCreated = Scheduler::CreateProcess(Process, "12345");
        if (processCreated)
        {
            processCreated = Scheduler::CreateProcess(Process, "abcde");
            if (processCreated)
            {
                while (true)
                {
                    Scheduler::Schedule();
                }
            }
            else
            {
                MiniUART::SendString("Error while starting process 2");
            }
            
        }
        else
        {
            MiniUART::SendString("Error while starting process 1");
        }
        

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        CallStaticDestructors();

        UnitTests::RunPostStaticDestructors();
    }
}