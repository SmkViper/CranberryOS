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

        void MallocNonNullTest()
        {
            auto const testPage = AutoKernelPage{};
            auto testAllocator = ::MemoryManagement::SimpleAllocator{ testPage.QPage() };

            auto* pinteger = std::bit_cast<uint32_t*>(testAllocator.Malloc(sizeof(uint32_t)));
            EmitTestResult((pinteger != nullptr), "SimpleAllocator Malloc Returns Non-null");
            testAllocator.Free(pinteger);
        }

        // #TODO: Need a lot more tests
        
    } // anonymous namespace

    // #TODO: Figure out if there is a better way to fix this
    // Unclear why clang-tidy thinks internal linkage works here, as it's called from Framework.cpp
    // NOLINTNEXTLINE(misc-use-internal-linkage)
    void Run()
    {
        MallocNonNullTest();
    }
} // UnitTests::MemoryManagement::SimpleAllocator namespace