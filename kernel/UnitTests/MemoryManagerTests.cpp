#include "MemoryManagerTests.h"

#include "../MemoryManager.h"
#include "../PointerTypes.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
namespace UnitTests::MemoryManager
{
    namespace
    {
        static_assert(::MemoryManager::CalculateBlockStart(0x1, 0x1000) == 0x0, "Unexpected block start");
        static_assert(::MemoryManager::CalculateBlockStart(PhysicalPtr{ 0x1 }, 0x1000) == PhysicalPtr{ 0x0 }, "Unexpected block start");
        static_assert(::MemoryManager::CalculateBlockStart(VirtualPtr{ 0x1 }, 0x1000) == VirtualPtr{ 0x0 }, "Unexpected block start");

        static_assert(::MemoryManager::CalculateBlockEnd(0x1, 0x1000) == 0x0FFF, "Unexpected block end");
        static_assert(::MemoryManager::CalculateBlockEnd(PhysicalPtr{ 0x1 }, 0x1000) == PhysicalPtr{ 0x0FFF }, "Unexpected block end");
        static_assert(::MemoryManager::CalculateBlockEnd(VirtualPtr{ 0x1 }, 0x1000) == VirtualPtr{ 0x0FFF }, "Unexpected block end");

        static_assert(::MemoryManager::CalculateBlockStart(0x1024, 0x1000) == 0x1000, "Unexpected block start");
        static_assert(::MemoryManager::CalculateBlockStart(PhysicalPtr{ 0x1024 }, 0x1000) == PhysicalPtr{ 0x1000 }, "Unexpected block start");
        static_assert(::MemoryManager::CalculateBlockStart(VirtualPtr{ 0x1024 }, 0x1000) == VirtualPtr{ 0x1000 }, "Unexpected block start");

        static_assert(::MemoryManager::CalculateBlockEnd(0x1024, 0x1000) == 0x1FFF, "Unexpected block end");
        static_assert(::MemoryManager::CalculateBlockEnd(PhysicalPtr{ 0x1024 }, 0x1000) == PhysicalPtr{ 0x1FFF }, "Unexpected block end");
        static_assert(::MemoryManager::CalculateBlockEnd(VirtualPtr{ 0x1024 }, 0x1000) == VirtualPtr{ 0x1FFF }, "Unexpected block end");

        // #TODO: AllocateKernelPage tests
        // #TODO: AllocateUserPage tests
        // #TODO: CopyVirtualMemory tests

        // SetPageGlobalDirectory modifies system state in a way that would likely screw things up, so not really
        // able to be tested
    }

    void Run()
    {
        // #TODO: No tests yet, probably will need some ones later
    }
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)