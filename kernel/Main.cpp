#include "AArch64/MMUDefines.h"
#include "Peripherals/DeviceTree.h"
#include "ExceptionVectorHandlers.h"
#include "IRQ.h"
#include "Main.h"
#include "MiniUart.h"
#include "Print.h"
#include "Scheduler.h"
#include "Timer.h"
#include "UnitTests.h"
#include "user_Program.h"
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

    // Defined by the linker to point at the start and end of the user "program" embedded in our image
    extern void* __user_start;
    extern void* __user_end;
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
     * Process trampoline which will move to user mode
     * 
     * @param apParam The parameter sent to the process
     */
    void KernelProcess(const void* const /*apParam*/)
    {
        Print::FormatToMiniUART("Kernel process started. EL {}\r\n", CPU::GetExceptionLevel());
        const auto pbegin = &__user_start;
        const auto size = reinterpret_cast<uintptr_t>(&__user_end) - reinterpret_cast<uintptr_t>(pbegin);
        const auto processOffset = reinterpret_cast<uintptr_t>(&User::Process) - reinterpret_cast<uintptr_t>(pbegin);

        const auto succeeded = Scheduler::MoveToUserMode(pbegin, size, processOffset);
        if (!succeeded)
        {
            MiniUART::SendString("Error while moving process to user mode\r\n");
        }
        // User::Process will run after we return, now that we've set up our process as a user one
    }
}

// Called from assembly, so don't mangle the name
extern "C"
{
    // #TODO: Handle to the "global shared object" that clang apparently wants? Not sure what its for yet, but without
    // it there are linker issues with it thinking the object is out of range
    void* __dso_handle;

}

namespace Kernel
{
    void kmain(uint32_t const aDTBPointer, uint64_t const aX1Reserved, uint64_t const aX2Reserved,
        uint64_t const aX3Reserved, uint32_t const aStartPointer)
    {
        CallStaticConstructors();

        MiniUART::Init();
        irq_vector_init();
        Scheduler::InitTimer();
        ExceptionVectors::EnableInterruptController();
        enable_irq();

        Print::FormatToMiniUART("DTB Address: {:x}\r\n", aDTBPointer);
        Print::FormatToMiniUART("x1: {:x}\r\n", aX1Reserved);
        Print::FormatToMiniUART("x2: {:x}\r\n", aX2Reserved);
        Print::FormatToMiniUART("x3: {:x}\r\n", aX3Reserved);
        Print::FormatToMiniUART("_start: {:x}\r\n", aStartPointer);
        // #TODO: Should find a better way to go from the pointer from the firmware to our virtual address
        //DeviceTree::ParseDeviceTree(reinterpret_cast<uint8_t const*>(static_cast<uintptr_t>(aDTBPointer) + VA_START));

        // #TODO: Fix crashing tests by implementing MMU support
        // Tests currently crash on real hardware due to unaligned access (pointers to strings being dereferenced into
        // a 'x' register, which requires higher alignment than 1). -mno-unaligned-access does not seem to fix the
        // problem. The issue _should_ go away once we support virtual memory.
        // The tests work on QEMU, so this can be uncommented for QEMU runs to ensure certain things work.
        //UnitTests::Run();

        const auto clockFrequencyHz = Timing::GetSystemCounterClockFrequencyHz();
        Print::FormatToMiniUART("System clock freq: {}hz\r\n", clockFrequencyHz);

        const auto processID = Scheduler::CopyProcess(Scheduler::CreationFlags::KernelThreadC, KernelProcess, nullptr);
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