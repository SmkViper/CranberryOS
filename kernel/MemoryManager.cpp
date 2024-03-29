#include "MemoryManager.h"

#include <bit>
#include <bitset>
#include <cstdint>
#include <cstring>
#include "AArch64/MemoryDescriptor.h"
#include "AArch64/MemoryPageTables.h"
#include "PointerTypes.h"
#include "Scheduler.h"
#include "TaskStructs.h"
#include "Utils.h"

extern "C"
{
    // from link.ld
    extern uint8_t const _kernel_image_end[];

    // Functions defined in MemoryManager.S
    /**
     * Set the current page global directory
     * 
     * @param apNewPGD Pointer to the new page global directory
     */
    void set_pgd(void const* apNewPGD);
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
        PhysicalPtr CalculatePagingMemoryPAStart()
        {
            // #TODO: Why is _kernel_image_end here a virtual address when in the boot process it's a physical
            // address?
            auto const kernalImageEndVA = std::bit_cast<uintptr_t>(&_kernel_image_end);
            auto const kernelImageEndPA = PhysicalPtr{ kernalImageEndVA - KernelVirtualAddressOffset };
            return CalculateBlockEnd(kernelImageEndPA, L2BlockSize).Offset(1);
        }

        constexpr auto PageMask = ~(PageSize - 1);

        // #TODO: Hardcoding only 64 pages for now, we need something better for this (probably once we calculate what
        // is available from the device tree)
        constexpr auto MaxPageCount = 64U;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::bitset<MaxPageCount> PageInUse;

        /**
         * Allocate a page of memory
         * 
         * @return Physical address of the new allocated page of memory, zeroed out
         */
        PhysicalPtr GetFreePage()
        {
            // Very simple for now, just find the first unused page and return it
            auto const pageMemoryStartPA = CalculatePagingMemoryPAStart();
            for (auto curPage = 0ULL; curPage < PageInUse.size(); ++curPage)
            {
                if (!PageInUse[curPage])
                {
                    PageInUse[curPage] = true;
                    auto newPageStartPA = pageMemoryStartPA.Offset(curPage * PageSize);
                    // have to add the KernelVirtualAddressStart because that's where the physical address is mapped to
                    // in kernel space
                    memset(std::bit_cast<void*>(newPageStartPA.Offset(KernelVirtualAddressOffset).GetAddress()), 0, PageSize);
                    return newPageStartPA;
                }
            }
            return PhysicalPtr{};
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
        LowerTableViewT MapTable(TableViewT aTable, VirtualPtr const aUserVirtualAddress, bool& arNewTable)
        {
            // #TODO: Can we make a relationship between TableViewT and LowerTableViewT so it can be deduced?

            arNewTable = false; // assume we don't need a new table

            auto const entry = aTable.GetEntryForVA(aUserVirtualAddress);
            PhysicalPtr pagePA;
            entry.Visit(Overloaded{
                [&arNewTable, &pagePA, aTable, aUserVirtualAddress](AArch64::Descriptor::Fault)
                {
                    // this part hasn't been set up yet, so add an entry
                    arNewTable = true;

                    AArch64::Descriptor::Table tableDescriptor;
                    pagePA = GetFreePage();
                    tableDescriptor.Address(pagePA);

                    aTable.SetEntryForVA(aUserVirtualAddress, tableDescriptor);
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

            // virtual address for memory in the kernel is physical address plus offset
            auto* const ppageVA = std::bit_cast<uint64_t*>(pagePA.Offset(KernelVirtualAddressOffset).GetAddress());
            return LowerTableViewT{ ppageVA };
        }

        /**
         * Map a new table entry into the page table
         * 
         * @param aTableVirtualAddress Kernel virtual address for the table
         * @param aUserVirtualAddress User virtual address we want to map
         * @param aPhysicalPage The physical page to map
         */
        void MapTableEntry(AArch64::PageTable::Level3View const aTable, const VirtualPtr aUserVirtualAddress, PhysicalPtr const aPhysicalPage)
        {
            AArch64::Descriptor::Page pageDescriptor;
            pageDescriptor.Address(aPhysicalPage);
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
         * @param aPhysicalPage The physical page the virtual page should map to
         */
        void MapPage(Scheduler::TaskStruct& arTask, VirtualPtr const aVirtualAddress, PhysicalPtr const aPhysicalPage)
        {
            if (arTask.MemoryState.PageGlobalDirectory == PhysicalPtr{})
            {
                arTask.MemoryState.PageGlobalDirectory = GetFreePage();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = arTask.MemoryState.PageGlobalDirectory;
                ++arTask.MemoryState.KernelPagesCount;
            }

            // helper to convert a table's pointer to the physical memory address, assuming offset mapping
            auto tablePtrToPAOffset = [](uint64_t const* const apTable)
            {
                return PhysicalPtr{ std::bit_cast<uintptr_t>(apTable) - KernelVirtualAddressOffset};
            };

            // PGD addresses are offset-mapped to virtual addresses
            auto const pageGlobalDirectoryVA = VirtualPtr{ arTask.MemoryState.PageGlobalDirectory.GetAddress() }.Offset(KernelVirtualAddressOffset);
            auto const pageGlobalDirectory = AArch64::PageTable::Level0View{ std::bit_cast<uint64_t*>(pageGlobalDirectoryVA.GetAddress()) };
            auto newTable = false;
            auto const pageUpperDirectory = MapTable<AArch64::PageTable::Level1View>(pageGlobalDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAOffset(pageUpperDirectory.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            auto const pageMiddleDirectory = MapTable<AArch64::PageTable::Level2View>(pageUpperDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAOffset(pageMiddleDirectory.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            auto const pageTableEntry = MapTable<AArch64::PageTable::Level3View>(pageMiddleDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAOffset(pageTableEntry.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            MapTableEntry(pageTableEntry, aVirtualAddress, aPhysicalPage);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            arTask.MemoryState.UserPages[arTask.MemoryState.UserPagesCount] = Scheduler::UserPage{ aPhysicalPage, aVirtualAddress };
            ++arTask.MemoryState.UserPagesCount;
        }
    }

    void* AllocateKernelPage()
    {
        auto const physicalPage = GetFreePage();
        if (physicalPage == PhysicalPtr{})
        {
            return nullptr;
        }
        // map the physical page to the kernel address space (offset-mapped)
        return std::bit_cast<void*>(physicalPage.Offset(KernelVirtualAddressOffset).GetAddress());
    }

    void* AllocateUserPage(Scheduler::TaskStruct& arTask, VirtualPtr const aVirtualAddress)
    {
        auto const physicalPage = GetFreePage();
        if (physicalPage == PhysicalPtr{})
        {
            return nullptr;
        }

        MapPage(arTask, aVirtualAddress, physicalPage);
        // map the physical page to the kernel address space (offset-mapped)
        return std::bit_cast<void*>(physicalPage.Offset(KernelVirtualAddressOffset).GetAddress());
    }

    bool CopyVirtualMemory(Scheduler::TaskStruct& arDestinationTask, const Scheduler::TaskStruct& aCurrentTask)
    {
        for (auto curPage = 0U; curPage < aCurrentTask.MemoryState.UserPagesCount; ++curPage)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            auto* const pkernelVA = AllocateUserPage(arDestinationTask, aCurrentTask.MemoryState.UserPages[curPage].VirtualAddress);
            if (pkernelVA == nullptr)
            {
                return false;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            memcpy(pkernelVA, std::bit_cast<const void*>(aCurrentTask.MemoryState.UserPages[curPage].VirtualAddress.GetAddress()), PageSize);
        }
        return true;
    }

    void SetPageGlobalDirectory(PhysicalPtr const aNewPGD)
    {
        set_pgd(std::bit_cast<void const*>(aNewPGD.GetAddress()));
    }
}

// Called from assembler
extern "C"
{
    /**
     * Called when a EL0 data abort fault is triggered
     * 
     * @param aAddress The faulting address
     * @param aESR The value of the ESR_EL1 register
     * @return 0 if it was handled, non-zero if it was not
     */
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    int do_mem_abort(uintptr_t const aAddress, uintptr_t const aESR)
    {
        constexpr auto dfscMask = 0b11'1111U;
        const auto dataFaultStatusCode = aESR & dfscMask;
        // Translation faults are: 100, 101, 110, and 111 depending on the level
        // We only care if any translation fault occurs, not which specific level, so we only make sure bit 3 is set
        // and no higher bits in the status code are
        constexpr auto anyTranslationFaultMask = 0b11'1100U;
        constexpr auto anyTranslationFault = 0b100U;
        if ((dataFaultStatusCode & anyTranslationFaultMask) == anyTranslationFault)
        {
            const auto newPage = MemoryManager::GetFreePage();
            if (newPage == PhysicalPtr{})
            {
                return -1;
            }

            MemoryManager::MapPage(Scheduler::GetCurrentTask(), VirtualPtr{ aAddress & MemoryManager::PageMask }, newPage);
            return 0;
        }
        return -1;
    }
}