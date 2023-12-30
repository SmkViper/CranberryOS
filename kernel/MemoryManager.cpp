#include "MemoryManager.h"

#include <cstdint>
#include <cstring>
#include "AArch64/MemoryDescriptor.h"
#include "Peripherals/Base.h"
#include "Scheduler.h"
#include "TaskStructs.h"
#include "Utils.h"

extern "C"
{
    // from link.ld
    extern uint8_t __kernel_image_end[];

    // Functions defined in MemoryManager.S
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
        /**
         * Calculates the start of paging memory based on the end of the kernel image
         * 
         * @return The physical address that starts our paging memory
        */
        uintptr_t CalculatePagingMemoryPAStart()
        {
            // #TODO: Need new types for virtual/physical addresses
            // #TODO: Why is __kernel_image_end here a virtual address when in the boot process it's a physical
            // address?
            return CalculateBlockEnd(reinterpret_cast<uintptr_t>(__kernel_image_end) - KernelVirtualAddressStart.GetAddress(), L2BlockSize) + 1;
        }

        constexpr auto PageMask = ~(PageSize - 1);

        // #TODO: Hardcoding only 64 pages for now, we need something better for this (probably once we calculate what
        // is available from the device tree)
        std::bitset<64> PageInUse;

        /**
         * Allocate a page of memory
         * 
         * @return Physical address of the new allocated page of memory, zeroed out
         */
        void* GetFreePage()
        {
            // Very simple for now, just find the first unused page and return it
            auto const pageMemoryStartPA = CalculatePagingMemoryPAStart();
            for (auto curPage = 0ull; curPage < PageInUse.size(); ++curPage)
            {
                if (!PageInUse[curPage])
                {
                    PageInUse[curPage] = true;
                    auto newPageStartPA = pageMemoryStartPA + (curPage * PageSize); // physical address
                    // have to add the KernelVirtualAddressStart because that's where the physical address is mapped to
                    // in kernel space
                    memset(reinterpret_cast<void*>(newPageStartPA + KernelVirtualAddressStart.GetAddress()), 0, PageSize);
                    return reinterpret_cast<void*>(newPageStartPA);
                }
            }
            return nullptr;
        }

        /**
         * Free a page of memory
         * 
         * @param apPage Physical address of the page to free
         */
        /* Currently unused
        void FreePage(void* apPage)
        {
            // #TODO: Double-check that the page is valid
            auto const pageMemoryStart = CalculatePagingMemoryPAStart();
            const auto index = (reinterpret_cast<uintptr_t>(apPage) - pageMemoryStart) / PageSize;
            PageInUse[index] = false;
        }
        */

       /**
         * Map a new table, or get the existing table for the specified table, shift, and virtual address
         * 
         * @param aTable The table to add the entry to
         * @param aUserVirtualAddress The user virtual address we want to map
         * @param arNewTable OUT: Set to true if a new table had to be made, otherwise false
         * @return The table for the specified address - new or existing
         */
        template<class LowerTableViewT, class TableViewT>
        LowerTableViewT MapTable(TableViewT aTable, const uintptr_t aUserVirtualAddress, bool& arNewTable)
        {
            // #TODO: Can we make a relationship between TableViewT and LowerTableViewT so it can be deduced?

            arNewTable = false; // assume we don't need a new table

            auto const entry = aTable.GetEntryForVA(VirtualPtr{ aUserVirtualAddress });
            uintptr_t pagePA = 0;
            entry.Visit(Overloaded{
                [&arNewTable, &pagePA, aTable, aUserVirtualAddress](AArch64::Descriptor::Fault)
                {
                    // this part hasn't been set up yet, so add an entry
                    arNewTable = true;

                    AArch64::Descriptor::Table tableDescriptor;
                    pagePA = reinterpret_cast<uintptr_t>(GetFreePage());
                    tableDescriptor.Address(pagePA);

                    aTable.SetEntryForVA(VirtualPtr{ aUserVirtualAddress }, tableDescriptor);
                },
                [&pagePA](AArch64::Descriptor::Table aTableDescriptor)
                {
                    pagePA = aTableDescriptor.Address();
                },
                [](AArch64::Descriptor::L1Block)
                {
                    // #TODO: Panic if this ever happens
                },
                [](AArch64::Descriptor::L2Block)
                {
                    // #TODO: Panic if this ever happens
                },
                [](AArch64::Descriptor::Page)
                {
                    // #TODO: Panic if this ever happens
                }
            });

            auto const pageVA = pagePA + KernelVirtualAddressStart.GetAddress();
            return LowerTableViewT{ reinterpret_cast<uint64_t*>(pageVA) };
        }

        /**
         * Map a new table entry into the page table
         * 
         * @param aTableVirtualAddress Kernel virtual address for the table
         * @param aUserVirtualAddress User virtual address we want to map
         * @param apPhysicalPage The physical page to map
         */
        void MapTableEntry(AArch64::PageTable::Level3View const aTable, const VirtualPtr aUserVirtualAddress, void* const apPhysicalPage)
        {
            AArch64::Descriptor::Page pageDescriptor;
            pageDescriptor.Address(reinterpret_cast<uintptr_t>(apPhysicalPage));
            pageDescriptor.AttrIndx(NormalMAIRIndex); // normal memory
            pageDescriptor.AF(true); // don't trap on access
            pageDescriptor.AP(AArch64::Descriptor::Page::AccessPermissions::KernelRWUserRW); // let user r/w it
            
            aTable.SetEntryForVA(aUserVirtualAddress, pageDescriptor);
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

            // helper to convert a table's pointer to the physical memory address, assuming identity mapping
            auto tablePtrToPAIdentity = [](uint64_t const* const apTable)
            {
                // #TODO: Should by PhysicalPtr
                return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(apTable) - KernelVirtualAddressStart.GetAddress());
            };

            auto const pageGlobalDirectoryVA = reinterpret_cast<uintptr_t>(arTask.MemoryState.pPageGlobalDirectory) + KernelVirtualAddressStart.GetAddress();
            auto const pageGlobalDirectory = AArch64::PageTable::Level0View{ reinterpret_cast<uint64_t*>(pageGlobalDirectoryVA) };
            auto newTable = false;
            auto const pageUpperDirectory = MapTable<AArch64::PageTable::Level1View>(pageGlobalDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAIdentity(pageUpperDirectory.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            const auto pageMiddleDirectory = MapTable<AArch64::PageTable::Level2View>(pageUpperDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAIdentity(pageMiddleDirectory.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            const auto pageTableEntry = MapTable<AArch64::PageTable::Level3View>(pageMiddleDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAIdentity(pageTableEntry.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            MapTableEntry(pageTableEntry, VirtualPtr{ aVirtualAddress }, apPhysicalPage);
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
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pphysicalPage) + KernelVirtualAddressStart.GetAddress());
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
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pphysicalPage) + KernelVirtualAddressStart.GetAddress());
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

            MemoryManager::MapPage(Scheduler::GetCurrentTask(), aAddress & MemoryManager::PageMask, pnewPage);
            return 0;
        }
        return -1;
    }
}