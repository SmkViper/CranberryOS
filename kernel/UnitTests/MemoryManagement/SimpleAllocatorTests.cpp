#include "../../MemoryManagement/SimpleAllocator.h"

#include <bit>
#include <cstdint>
#include "../../MemoryManager.h"
#include "../Framework.h"

namespace UnitTests::MemoryManagement::SimpleAllocator
{
    namespace
    {
        // #TODO: Replace with unique_ptr when we have it
        struct [[nodiscard]] AutoKernelPage
        {
            /**
             * Allocates and holds onto a page of kernel memory
             */
            AutoKernelPage()
                : pPage{ MemoryManager::AllocateKernelPage() }
            {}

            /**
             * Frees the held page
             */
            ~AutoKernelPage()
            {
                MemoryManager::FreeKernelPage(pPage);
            }

            // Disable copy/move and assign
            AutoKernelPage(AutoKernelPage const&) = delete;
            AutoKernelPage(AutoKernelPage&&) noexcept = delete;
            AutoKernelPage& operator=(AutoKernelPage const&) = delete;
            AutoKernelPage& operator=(AutoKernelPage&&) noexcept = delete;

            /**
             * Obtains the held page
             * 
             * @return The held page
             */
            [[nodiscard]] void* QPage() const {return pPage;}

        private:
            void* pPage = nullptr;
        };

        /**
         * Testing basic malloc/free behavior
         */
        void MallocFreeTests()
        {
            auto const testPage = AutoKernelPage{};
            auto testAllocator = ::MemoryManagement::SimpleAllocator{ testPage.QPage() };

            auto* const poriginalMemory = testAllocator.Malloc(sizeof(uint32_t));
            EmitTestResult(poriginalMemory != nullptr, "SimpleAllocator::Malloc returns non-null");

            // read/write to ensure we have access to the memory
            constexpr auto testValue = 0xFEFE'ABCDU;
            // using volatile to try to ensure the values are written and read and not cached
            auto volatile* const ptestMemory = std::bit_cast<uint32_t volatile*>(poriginalMemory);
            *ptestMemory = testValue;
            EmitTestResult(*ptestMemory == testValue, "SimpleAllocator::Malloc memory read/write");

            auto* const potherMemory = testAllocator.Malloc(sizeof(uint32_t));
            EmitTestResult(potherMemory != poriginalMemory, "SimpleAllocator::Malloc returns seperate pointer");

            testAllocator.Free(poriginalMemory);

            auto* const preusedMemory = testAllocator.Malloc(sizeof(uint32_t));
            EmitTestResult(preusedMemory == poriginalMemory, "SimpleAllocator::Malloc reused free memory");

            testAllocator.Free(preusedMemory);
            testAllocator.Free(potherMemory);
            // Additional testing is done implicitly via the allocator destructor, which panics if there is any unfreed
            // or corrupted memory blocks
        }
    } // anonymous namespace

    // #TODO: Figure out if there is a better way to fix this
    // Unclear why clang-tidy thinks internal linkage works here, as it's called from Framework.cpp
    // NOLINTNEXTLINE(misc-use-internal-linkage)
    void Run()
    {
        MallocFreeTests();
    }
} // UnitTests::MemoryManagement::SimpleAllocator namespace