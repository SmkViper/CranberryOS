#include "Main.h"

#include <bit>
#include <cstdint>
#include "AArch64/CPU.h"
#include "UnitTests/Framework.h"
#include "ExceptionVectorHandlers.h"
#include "IRQ.h"
#include "MiniUart.h"
#include "PointerTypes.h"
#include "Print.h"
#include "Scheduler.h"
#include "user_Program.h"
#include "Utils.h"

// Uncomment define to output the device tree to UART on boot
//#define OUTPUT_DEVICE_TREE
#ifdef OUTPUT_DEVICE_TREE
#include "Peripherals/DeviceTree.h"
#include "MemoryManager.h"
#endif // OUTPUT_DEVICE_TREE

namespace
{
    using StaticInitFunction = void (*)();
    using StaticFiniFunction = void (*)();
}

extern "C"
{
    // Defined by the linker to point at the start/end of the init/fini arrays
    extern uintptr_t const _init_start[];
    extern uintptr_t const _init_end[]; // past the end
    extern uintptr_t const _fini_start[];
    extern uintptr_t const _fini_end[]; // past the end

    // Defined by the linker to point at the start and end of the user "program" embedded in our image
    extern void const* const _user_start;
    extern void const* const _user_end; // past the end
}

namespace
{
    /**
     * Calls all static constructors in our .init_array section
     */
    void CallStaticConstructors()
    {
        auto const init_arraySize = std::bit_cast<uintptr_t>(&_init_end) - std::bit_cast<uintptr_t>(&_init_start);
        for (auto curFunc = 0U; curFunc < init_arraySize; ++curFunc)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
            reinterpret_cast<StaticInitFunction>(_init_start[curFunc])();
        }
    }

    /**
     * Calls all static destructors in our .fini_array section
     */
    void CallStaticDestructors()
    {
        auto const fini_arraySize = std::bit_cast<uintptr_t>(&_fini_end) - std::bit_cast<uintptr_t>(&_fini_start);
        for (auto curFunc = 0U; curFunc < fini_arraySize; ++curFunc)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
            reinterpret_cast<StaticFiniFunction>(_fini_start[curFunc])();
        }
    }

    /**
     * Process trampoline which will move to user mode
     * 
     * @param apParam The parameter sent to the process
     */
    void KernelProcess(const void* const /*apParam*/)
    {
        // Splitting out all the bit_casts into their own lines because they appear to crash clang-tidy in some cases
        Print::FormatToMiniUART("Kernel process started. EL {}\r\n", static_cast<uint32_t>(AArch64::CPU::GetCurrentExceptionLevel()));
        auto const startOfUserCode = std::bit_cast<uintptr_t>(&_user_start);
        auto const endOfUserCode = std::bit_cast<uintptr_t>(&_user_end);
        auto const size = endOfUserCode - startOfUserCode;

        auto const processFnAddr = std::bit_cast<uintptr_t>(&User::Process);
        auto const processOffset = processFnAddr - startOfUserCode;

        auto const succeeded = Scheduler::MoveToUserMode(&_user_start, size, processOffset);
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
    // NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,cppcoreguidelines-avoid-non-const-global-variables)
    void* __dso_handle;
}

namespace Kernel
{
    void kmain(PhysicalPtr const aDTBPointer, uint64_t const aX1Reserved, uint64_t const aX2Reserved,
        uint64_t const aX3Reserved, PhysicalPtr const aStartPointer)
    {
        CallStaticConstructors();

        MiniUART::Init();
        irq_vector_init();
        Scheduler::InitTimer();
        ExceptionVectors::EnableInterruptController();
        enable_irq();

        Print::FormatToMiniUART("DTB Address: {}\r\n", aDTBPointer);
        Print::FormatToMiniUART("x1: {:x}\r\n", aX1Reserved);
        Print::FormatToMiniUART("x2: {:x}\r\n", aX2Reserved);
        Print::FormatToMiniUART("x3: {:x}\r\n", aX3Reserved);
        Print::FormatToMiniUART("_start: {}\r\n", aStartPointer);
        // #TODO: Should find a better way to go from the pointer from the firmware to our virtual address
#ifdef OUTPUT_DEVICE_TREE
        DeviceTree::ParseDeviceTree(reinterpret_cast<uint8_t const*>(aDTBPointer.GetAddress() + MemoryManager::KernelVirtualAddressOffset));
#endif // OUTPUT_DEVICE_TREE

        UnitTests::Run();

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