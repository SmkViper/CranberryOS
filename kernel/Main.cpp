#include "ExceptionVectorHandlers.h"
#include "IRQ.h"
#include "MiniUart.h"
#include "Print.h"
#include "Scheduler.h"
#include "SystemCall.h"
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
     * A "user process" that just outputs the string it is given repeatedly
     * 
     * @param apArray The string to output
     */
    void UserProcess1(const void* apArray)
    {
        const char* pstring = reinterpret_cast<const char*>(apArray);
        char buffer[] = {'\0', '\0'};
        while (true)
        {
            for (auto curIndex = 0u; pstring[curIndex] != '\0'; ++curIndex)
            {
                buffer[0] = pstring[curIndex];
                SystemCall::UARTWrite(buffer);
                Timing::Delay(100000);
            }
        }
    }

    /**
     * A user process that sets up two "user processes" to run in parallel (really threads, but still)
     */
    void UserProcess()
    {
        SystemCall::UARTWrite("User process started\r\n");
        auto pstack = SystemCall::AllocatePage();
        if (pstack == nullptr)
        {
            SystemCall::UARTWrite("Error while allocating stack for process 1\r\n");
            return;
        }
        auto processID = SystemCall::CreateProcess(UserProcess1, "12345", pstack);
        if (processID < 0)
        {
            SystemCall::UARTWrite("Error while creating process 1\r\n");
            return;
        }

        pstack = SystemCall::AllocatePage();
        if (pstack == nullptr)
        {
            SystemCall::UARTWrite("Error while allocating stack for process 2\r\n");
            return;
        }
        processID = SystemCall::CreateProcess(UserProcess1, "abcd", pstack);
        if (processID < 0)
        {
            SystemCall::UARTWrite("Error while creating process 2\r\n");
            return;
        }
        SystemCall::Exit();
    }
    
    /**
     * Process trampoline which will move to user mode
     * 
     * @param apParam The parameter sent to the process
     */
    void KernelProcess(const void* const /*apParam*/)
    {
        Print::FormatToMiniUART("Kernel process started. EL {}\r\n", CPU::GetExceptionLevel());
        const auto succeeded = Scheduler::MoveToUserMode(&UserProcess);
        if (!succeeded)
        {
            MiniUART::SendString("Error while moving process to user mode\r\n");
        }
        // UserProcess will run after we return, now that we've set up our process as a user one
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

        const auto processID = Scheduler::CreateProcess(Scheduler::CreationFlags::KernelThreadC, KernelProcess, nullptr, nullptr);
        if (processID >= 0)
        {
            while (true)
            {
                Scheduler::Schedule();
            }
        }
        else
        {
            MiniUART::SendString("Error while starting kernel process");
        }
        

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        CallStaticDestructors();

        UnitTests::RunPostStaticDestructors();
    }
}