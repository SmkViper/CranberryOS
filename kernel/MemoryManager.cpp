#include "MemoryManager.h"

#include <bit>
#include <bitset>
#include <cstdint>
#include <cstring>
#include "AArch64/MemoryDescriptor.h"
#include "AArch64/MemoryPageTables.h"
#include "AArch64/SystemRegisters.h"
#include "PointerTypes.h"
#include "Print.h"
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
        template<class TableViewT>
        auto MapTable(TableViewT aTable, VirtualPtr const aUserVirtualAddress, bool& arNewTable) -> AArch64::PageTable::ChildTableView_t<TableViewT>
        {
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
            return AArch64::PageTable::ChildTableView_t<TableViewT>{ ppageVA };
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
            auto const pageUpperDirectory = MapTable(pageGlobalDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAOffset(pageUpperDirectory.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            auto const pageMiddleDirectory = MapTable(pageUpperDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAOffset(pageMiddleDirectory.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            auto const pageTableEntry = MapTable(pageMiddleDirectory, aVirtualAddress, newTable);
            if (newTable)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                arTask.MemoryState.KernelPages[arTask.MemoryState.KernelPagesCount] = tablePtrToPAOffset(pageTableEntry.GetTablePtr());
                ++arTask.MemoryState.KernelPagesCount;
            }

            MapTableEntry(pageTableEntry, aVirtualAddress, aPhysicalPage);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            arTask.MemoryState.UserPages[arTask.MemoryState.UserPagesCount] = Scheduler::UserPage{
                .PhysicalAddress = aPhysicalPage,
                .VirtualAddress = aVirtualAddress
            };
            ++arTask.MemoryState.UserPagesCount;
        }
    }

    namespace Internal
    {
        /**
         * Checks to see if the MMU is enabled or not
         * 
         * @return Whether the MMU is enabled or not
         */
        bool MMUEnabled()
        {
            auto const sctlr_el1 = AArch64::SCTLR_EL1::Read();
            return sctlr_el1.M();
        }
    }

    namespace Debug
    {
        namespace
        {
            struct MemoryRange
            {
                // #TODO: Remove this when we have std::optional
                /**
                 * Checks to see if a range is in progress or not
                 * 
                 * @return True if there is a valid range being interated over
                 */
                [[nodiscard]] bool RangeInProgress() const { return Start != NoRangeCS; }

                // #TODO: Remove this when we have std::optional
                /**
                 * Resets the range to represent no range
                 */
                void Reset() { Start = NoRangeCS; }

                /**
                 * Start a new range of memory using the given address and descriptor
                 * 
                 * @param aStart Start of the range
                 * @param aDescriptor The descriptor for the first page or block in the range
                 */
                template<typename DescriptorT>
                void StartNewRange(VirtualPtr const aStart, DescriptorT const& aDescriptor)
                {
                    Start = aStart;
                    End = Start.Offset(DescriptorT::SizeCS - 1ULL);
                    AttrIndx = aDescriptor.AttrIndx();
                    AP = static_cast<decltype(AP)>(aDescriptor.AP()); // #TODO: underlying_type
                    AF = aDescriptor.AF();
                }

                /**
                 * Attempts to extend the range, returning true if it succeeded
                 * 
                 * @param aStart Start of the memory the descriptor is for
                 * @param aDescriptor The descriptor to check
                 * @return True if the range was extended, false if it wasn't
                 */
                template<typename DescriptorT>
                [[nodiscard]] bool ExtendRange(VirtualPtr const aStart, DescriptorT const& aDescriptor)
                {
                    if (RangeInProgress() && (aStart == End.Offset(1ULL) && SameMemoryAttributes(aDescriptor)))
                    {
                        End = End.Offset(DescriptorT::SizeCS);
                        return true;
                    }
                    return false;
                }

                /**
                 * Checks to see if the given descriptor is pointing at the same type of memory as the range
                 * 
                 * @param aDescriptor Descriptor to check
                 * @return True if the memory attributes match
                 */
                template<typename DescriptorT>
                [[nodiscard]] bool SameMemoryAttributes(DescriptorT const& aDescriptor) const
                {
                    return (aDescriptor.AttrIndx() == AttrIndx) &&
                        (static_cast<decltype(AP)>(aDescriptor.AP()) == AP) && // #TODO: underlying_type
                        (aDescriptor.AF() == AF);
                }

                // #TODO: Remove this when we have std::optional
                constexpr static VirtualPtr NoRangeCS = VirtualPtr{ 0xFFFF'FFFF'FFFF'FFFF };

                VirtualPtr Start = NoRangeCS;
                VirtualPtr End = NoRangeCS;
                uint8_t AttrIndx = 0;
                uint8_t AP = 0;
                bool AF = false;
            };

            /**
             * Outputs the given range if it is valid
             * 
             * @param aRange Range to output
             */
            void OutputRangeIfValid(MemoryRange const& aRange)
            {
                if (aRange.RangeInProgress())
                {
                    Print::FormatToMiniUART("{} - {}:\r\n", aRange.Start, aRange.End);
                    Print::FormatToMiniUART("\tAttrIndx: {}\r\n", aRange.AttrIndx);
                    Print::FormatToMiniUART("\tAP: {}\r\n", aRange.AP);
                    Print::FormatToMiniUART("\tAF: {}\r\n", aRange.AF ? "true" : "false"); // #TODO: Update when we support bool format
                }
            }

            /**
             * Output the virtual address range to UART for the given view
             * 
             * @param aView The table to iterate
             * @param aViewRootVA The root virtual address of the view
             * @param arActiveRange The currently active range which will be updated
             */
            template<typename ViewT>
            void OutputKernelVARangesToUARTImpl(ViewT const aView, VirtualPtr const aViewRootVA, MemoryRange& arActiveRange)
            {
                // #TODO: Move to ranged for when possible
                for (auto curPageIndex = 0U; curPageIndex < AArch64::PageTable::PointersPerTable; ++curPageIndex)
                {
                    auto const entry = aView.GetEntry(curPageIndex);
                    auto const entryBaseAddr = ViewT::GetAddressForEntry(curPageIndex, aViewRootVA);
                    entry.Visit(Overloaded{
                        [&arActiveRange](AArch64::Descriptor::Fault)
                        {
                            OutputRangeIfValid(arActiveRange);
                            arActiveRange.Reset();
                        },
                        [&arActiveRange, entryBaseAddr](AArch64::Descriptor::Table const aTable)
                        {
                            if constexpr(AArch64::PageTable::HasChildTableView_v<ViewT>)
                            {
                                // assuming offset mapping
                                // #TODO: We can likely do better
                                auto const virtualTableAddress = VirtualPtr{ aTable.Address().GetAddress() }.Offset(KernelVirtualAddressOffset);
                                OutputKernelVARangesToUARTImpl(
                                    AArch64::PageTable::ChildTableView_t<ViewT>{ std::bit_cast<uint64_t*>(virtualTableAddress.GetAddress()) },
                                    entryBaseAddr,
                                    arActiveRange
                                );
                            }
                            else
                            {
                                OutputRangeIfValid(arActiveRange);
                                arActiveRange.Reset();

                                // #TODO: Should probably be an assert
                                Print::FormatToMiniUART("ERROR: No defined child table type for table at {}\r\n", entryBaseAddr);
                            }
                        },
                        [&arActiveRange, entryBaseAddr](auto const aBlockOrPage)
                        {
                            auto const rangeExtended = arActiveRange.ExtendRange(entryBaseAddr, aBlockOrPage);
                            if (!rangeExtended)
                            {
                                OutputRangeIfValid(arActiveRange);
                                arActiveRange.StartNewRange(entryBaseAddr, aBlockOrPage);
                            }
                        }
                    });
                }
            }

            /**
             * Outputs virtual address information to UART
             * 
             * @param aTable The table register to output from
             * @param aBaseAddr The base address for the given register (0 for base register 0, and the high bits set
             * for base register 1)
             */
            void OutputKernelVARangesToUART(AArch64::TTBRn_EL1 const aTable, VirtualPtr const aBaseAddr)
            {
                // #TODO: Probably want to make this more flexible to output to other locations, but for now UART is fine
                auto const physicalTableAddress = aTable.BADDR();
                // assuming offset mapping
                // #TODO: Need to handle this better
                auto const virtualTableAddress = VirtualPtr{ physicalTableAddress.GetAddress() }.Offset(KernelVirtualAddressOffset);

                MemoryRange range;
                OutputKernelVARangesToUARTImpl(
                    AArch64::PageTable::Level0View{ std::bit_cast<uint64_t*>(virtualTableAddress.GetAddress()) },
                    aBaseAddr,
                    range
                );
            }
        }

        void OutputKernelVARangesToUART()
        {
            OutputKernelVARangesToUART(AArch64::TTBRn_EL1::Read0(), VirtualPtr{ 0ULL });
            // #TODO: Better way to calculate the base address for TTBR1?
            OutputKernelVARangesToUART(AArch64::TTBRn_EL1::Read1(), VirtualPtr{ KernelVirtualAddressOffset });
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