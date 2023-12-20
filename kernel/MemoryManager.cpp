#include "MemoryManager.h"

#include <cstdint>
#include <cstring>
#include "AArch64/MemoryDescriptor.h"
#include "AArch64/MMUDefines.h"
#include "Peripherals/Base.h"
#include "Scheduler.h"
#include "TaskStructs.h"

// Functions defined in MemoryManager.S
extern "C"
{
    /**
     * Set the current page global directory
     * 
     * @param apNewPGD Pointer to the new page global directory
     */
    void set_pgd(const void* apNewPGD);
}

namespace MemoryManager
{
    namespace
    {
        constexpr auto LowMemoryC = LOW_MEMORY; // reserve 4mb of low memory, which is enough to cover our kernel
        // don't run into any of the memory-mapped perhipherals (NOT using the PeripheralBaseAddr because that's an
        // absolute addr, and we want relative for our paging calculations)
        constexpr auto HighMemoryC = DeviceBaseAddress;

        constexpr auto PagingMemoryC = HighMemoryC - LowMemoryC;
        constexpr auto PageCountC = PagingMemoryC / PageSize;

        constexpr auto PageMaskC = 0xffff'ffff'ffff'f000;

        bool PageInUse[PageCountC] = {false};

        /**
         * Allocate a page of memory
         * 
         * @return Physical address of the new allocated page of memory, zeroed out
         */
        void* GetFreePage()
        {
            // Very simple for now, just find the first unused page and return it
            for (auto curPage = 0ull; curPage < PageCountC; ++curPage)
            {
                if (!PageInUse[curPage])
                {
                    PageInUse[curPage] = true;
                    auto newPageStart = LowMemoryC + (curPage * PageSize); // physical address
                    // have to add the KernalVirtualAddressStart because that's where the physical address is mapped to
                    // in kernel space
                    memset(reinterpret_cast<void*>(newPageStart + KernalVirtualAddressStart), 0, PageSize);
                    return reinterpret_cast<void*>(newPageStart);
                }
            }
            return nullptr;
        }

        /**
         * Free a page of memory
         * 
         * @param apPage Physical address of the page to free
         */
        void FreePage(void* apPage)
        {
            // #TODO: Double-check that the page is valid
            const auto index = (reinterpret_cast<uintptr_t>(apPage) - LowMemoryC) / PageSize;
            PageInUse[index] = false;
        }

        /**
         * Map a new table, or get the existing table for the specified table, shift, and virtual address
         * 
         * @param aTableVirtualAddress Kernel virtual address for the table
         * @param aShift How much to shift the user virtual address to get the index into the table
         * @param aUserVirtualAddress The user virtual address we want to map
         * @param arNewTable OUT: Set to true if a new table had to be made, otherwise false
         * @return The table for the specified address - new or existing
         */
        void* MapTable(const uintptr_t aTableVirtualAddress, const uint64_t aShift, const uintptr_t aUserVirtualAddress, bool& arNewTable)
        {
            arNewTable = false; // assume we don't need a new table

            // each table is just an array of pointers
            auto ptable = reinterpret_cast<uintptr_t*>(aTableVirtualAddress);

            // mask out the bits for this particular table index
            const auto index = (aUserVirtualAddress >> aShift) & (PTRS_PER_TABLE - 1);
            if (ptable[index] == 0u)
            {
                // this part hasn't been set up yet, so add an entry
                arNewTable = true;

                AArch64::Descriptor::Table tableDescriptor;
                auto const pnewPage = GetFreePage();
                tableDescriptor.Address(reinterpret_cast<uintptr_t>(pnewPage));

                AArch64::Descriptor::Table::Write(tableDescriptor, ptable, index);

                return pnewPage;
            }
            return reinterpret_cast<void*>(ptable[index] & PageMaskC);
        }

        /**
         * Map a new table entry into the page table
         * 
         * @param aTableVirtualAddress Kernel virtual address for the table
         * @param aUserVirtualAddress User virtual address we want to map
         * @param apPhysicalPage The physical page to map
         */
        void MapTableEntry(const uintptr_t aTableVirtualAddress, const uintptr_t aUserVirtualAddress, void* const apPhysicalPage)
        {
            // each table is just an array of pointers
            auto ptable = reinterpret_cast<uintptr_t*>(aTableVirtualAddress);

            AArch64::Descriptor::Page pageDescriptor;
            pageDescriptor.Address(reinterpret_cast<uintptr_t>(apPhysicalPage));
            pageDescriptor.AttrIndx(MT_NORMAL_NC); // normal memory
            pageDescriptor.AF(true); // don't trap on access
            pageDescriptor.AP(AArch64::Descriptor::Page::AccessPermissions::KernelRWUserRW); // let user r/w it
            
            auto const index = (aUserVirtualAddress >> PAGE_SHIFT) & (PTRS_PER_TABLE - 1);
            AArch64::Descriptor::Page::Write(pageDescriptor, ptable, index);
        }

        /**
         * Maps a user page for the specified task
         * 
         * @param arTask The task the page is for
         * @param aVirtualAddress The user virtual address for the page
         * @param apPhysicalPage The physical page the virtual page should map to
         */
        void MapPage(Scheduler::TaskStruct& arTask, const uintptr_t aVirtualAddress, void* const apPhysicalPage)
        {
            if (arTask.MemoryState.pPageGlobalDirectory == nullptr)
            {
                arTask.MemoryState.pPageGlobalDirectory = GetFreePage();
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = arTask.MemoryState.pPageGlobalDirectory;
                ++arTask.MemoryState.KernelPagesCount;
            }

            const auto ppageGlobalDirectory = arTask.MemoryState.pPageGlobalDirectory;
            auto newTable = false;
            const auto ppageUpperDirectory = MapTable(reinterpret_cast<uintptr_t>(ppageGlobalDirectory) + KernalVirtualAddressStart, PGD_SHIFT, aVirtualAddress, newTable);
            if (newTable)
            {
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = ppageUpperDirectory;
                ++arTask.MemoryState.KernelPagesCount;
            }

            const auto ppageMiddleDirectory = MapTable(reinterpret_cast<uintptr_t>(ppageUpperDirectory) + KernalVirtualAddressStart, PUD_SHIFT, aVirtualAddress, newTable);
            if (newTable)
            {
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = ppageMiddleDirectory;
                ++arTask.MemoryState.KernelPagesCount;
            }

            const auto ppageTableEntry = MapTable(reinterpret_cast<uintptr_t>(ppageMiddleDirectory) + KernalVirtualAddressStart, PMD_SHIFT, aVirtualAddress, newTable);
            if (newTable)
            {
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = ppageTableEntry;
                ++arTask.MemoryState.KernelPagesCount;
            }

            MapTableEntry(reinterpret_cast<uintptr_t>(ppageTableEntry) + KernalVirtualAddressStart, aVirtualAddress, apPhysicalPage);
            arTask.MemoryState.UserPages[arTask.MemoryState.UserPagesCount] = Scheduler::UserPage{apPhysicalPage, aVirtualAddress};
            ++arTask.MemoryState.UserPagesCount;
        }
    }

    void* AllocateKernelPage()
    {
        const auto pphysicalPage = GetFreePage();
        if (pphysicalPage == nullptr)
        {
            return nullptr;
        }
        // map the physical page to the kernel address space
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pphysicalPage) + KernalVirtualAddressStart);
    }

    void* AllocateUserPage(Scheduler::TaskStruct& arTask, const uintptr_t aVirtualAddress)
    {
        const auto pphysicalPage = GetFreePage();
        if (pphysicalPage == nullptr)
        {
            return nullptr;
        }

        MapPage(arTask, aVirtualAddress, pphysicalPage);
        // map the physical page to the kernel address space
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pphysicalPage) + KernalVirtualAddressStart);
    }

    bool CopyVirtualMemory(Scheduler::TaskStruct& arDestinationTask, const Scheduler::TaskStruct& aCurrentTask)
    {
        for (auto curPage = 0u; curPage < aCurrentTask.MemoryState.UserPagesCount; ++curPage)
        {
            const auto pkernelVA = AllocateUserPage(arDestinationTask, aCurrentTask.MemoryState.UserPages[curPage].VirtualAddress);
            if (pkernelVA == nullptr)
            {
                return false;
            }
            memcpy(pkernelVA, reinterpret_cast<const void*>(aCurrentTask.MemoryState.UserPages[curPage].VirtualAddress), PageSize);
        }
        return true;
    }

    void SetPageGlobalDirectory(const void* const apNewPGD)
    {
        set_pgd(apNewPGD);
    }
}

// Called from assembler
extern "C"
{
    int do_mem_abort(uintptr_t aAddress, uintptr_t aESR)
    {
        const auto dataFaultStatusCode = aESR & 0b11'1111;
        // Translation faults are: 100, 101, 110, and 111 depending on the level
        if ((dataFaultStatusCode & 0b11'1100) == 0b100)
        {
            const auto pnewPage = MemoryManager::GetFreePage();
            if (pnewPage == nullptr)
            {
                return -1;
            }

            MemoryManager::MapPage(Scheduler::GetCurrentTask(), aAddress & MemoryManager::PageMaskC, pnewPage);
            return 0;
        }
        return -1;
    }
}