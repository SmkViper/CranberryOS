#include "MemoryManager.h"

#include <cstdint>
#include "ARM/MMUDefines.h"
#include "Peripherals/Base.h"

namespace MemoryManager
{
    namespace
    {
        // #TODO: Pulling from macros for now, hopefully can clean this up a lot later
        constexpr auto PageSizeC = PAGE_SIZE; // 4k pages
        constexpr auto LowMemoryC = LOW_MEMORY; // reserve 4mb of low memory, which is enough to cover our kernel
        constexpr auto HighMemoryC = MemoryMappedIO::PeripheralBaseAddr; // don't run into any of the memory-mapped perhipherals

        constexpr auto PagingMemoryC = HighMemoryC - LowMemoryC;
        constexpr auto PageCountC = PagingMemoryC / PageSizeC;

        bool PageInUse[PageCountC] = {false};
    }

    void* GetFreePage()
    {
        // Very simple for now, just find the first unused page and return it
        for (auto curPage = 0ull; curPage < PageCountC; ++curPage)
        {
            if (!PageInUse[curPage])
            {
                PageInUse[curPage] = true;
                return reinterpret_cast<void*>(LowMemoryC + (curPage * PageSizeC));
            }
        }
        return nullptr;
    }

    void FreePage(void* apPage)
    {
        // #TODO: Double-check that the page is valid
        const auto index = (reinterpret_cast<uintptr_t>(apPage) - LowMemoryC) / PageSizeC;
        PageInUse[index] = false;
    }
}