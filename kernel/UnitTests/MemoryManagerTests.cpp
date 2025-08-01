#include "MemoryManagerTests.h"

#include <bit>
#include <cstdint>
#include "Framework.h"
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

        /**
         * Tests around Allocate/FreeKernelPage
         */
        void AllocateAndFreeKernelPageTests()
        {
            auto* const poriginalTestPage = ::MemoryManager::AllocateKernelPage();
            EmitTestResult((poriginalTestPage != nullptr), "MemoryManager::AllocateKernelPage returns non-null");
            
            // read/write to ensure we have access to the memory
            constexpr auto testValue = 0xFEFE'ABCDU;
            // using volatile to try to ensure the values are written and read and not cached
            auto volatile* const ptestMemory = std::bit_cast<uint32_t volatile*>(poriginalTestPage);
            *ptestMemory = testValue;
            EmitTestResult(*ptestMemory == testValue, "MemoryManager::AllocateKernelPage memory read/write");

            auto* const potherPage = ::MemoryManager::AllocateKernelPage();
            EmitTestResult(potherPage != poriginalTestPage, "MemoryManager::AllocateKernelPage returns seperate page");

            // can't directly see if the page is freed, but we can ensure that the next allocate returns what was
            // previously freed
            ::MemoryManager::FreeKernelPage(poriginalTestPage);
            auto* const preusedTestPage = ::MemoryManager::AllocateKernelPage();
            EmitTestResult(preusedTestPage == poriginalTestPage, "MemoryManager::AllocateKernelPage reused free page");

            ::MemoryManager::FreeKernelPage(preusedTestPage);
            ::MemoryManager::FreeKernelPage(potherPage);
        }
        
        // #TODO: AllocateUserPage tests
        // #TODO: CopyVirtualMemory tests

        // SetPageGlobalDirectory modifies system state in a way that would likely screw things up, so not really
        // able to be tested
    }

    void Run()
    {
        AllocateAndFreeKernelPageTests();
    }
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)