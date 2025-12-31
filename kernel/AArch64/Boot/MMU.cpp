// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include "MMU.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include "../../Peripherals/DeviceTree.h"
#include "../../MemoryManager.h"
#include "../../PointerTypes.h"
#include "../../Utils.h"
#include "../MemoryDescriptor.h"
#include "../MemoryPageTables.h"
#include "../SystemRegisters.h"
#include "Output.h"

// Address translation documentation: https://documentation-service.arm.com/static/5efa1d23dbdee951c1ccdec5?token=

extern "C"
{
    // from link.ld

    // Disabling the non-const global warning cause they're markers to writable memory that we need to write to
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    extern uint8_t _pg_dir[];
    extern uint8_t _pg_dir_end[]; // past the end
    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    extern uint8_t const _kernel_image[];
    extern uint8_t const _kernel_image_end[];
}

namespace AArch64::Boot
{
    namespace
    {
        // We want to store the device tree at a known location so we don't have to worry about remapping it on boot
        // #TODO: Is there a better way to handle this - perhaps by mapping it on demand while it is needed instead of
        // copying it (since we might want the space back)
        constexpr auto const DeviceTreeStorageSizeCS = 2ULL * 1024ULL * 1024ULL; // 2MB
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-non-const-global-variables)
        uint8_t DeviceTreeStorageS[DeviceTreeStorageSizeCS] = {};

        /**
         * Calculates the offset from where things are linked to where they are in physical memory. Only works before
         * the MMU is turned on.
         * 
         * @return The offset from physical to linker address
         */
        uintptr_t CalculatePhysicalToLinkTimeAddressOffset()
        {
            uintptr_t physicalAddr = 0; // NOLINT(misc-const-correctness)
            uintptr_t linktimeAddr = 0; // NOLINT(misc-const-correctness)
            
            asm volatile( // NOLINT(hicpp-no-assembler)
                // Loads the PC-relative address of this instruction into physicalAddr
                "   adr %[physicalAddr], #0\n"
                // Loads the linktime address of this instruction into linktimeAddr
                "1: ldr %[linktimeAddr], =1b\n"
                : [physicalAddr] "=r"(physicalAddr),
                  [linktimeAddr] "=r"(linktimeAddr)
            );
            return linktimeAddr - physicalAddr - 4;
        }

        /**
         * Helper to specify a memory range where the end is exclusive
         */
        template<typename T>
        struct ExclusiveMemoryRange
        {
            /**
             * Construct a memory range
             * 
             * @param aBegin The start of the range
             * @param aEnd The end of the range (exclusive)
             */
            constexpr ExclusiveMemoryRange(T aBegin, T aEnd)
                : Begin{ aBegin }
                , End{ aEnd }
            {
                if (aBegin >= aEnd)
                {
                    PanicNoMMU("Begin should be before end");
                }
            }

            T Begin;
            T End;
        };

        /**
         * Helper to specify a memory range where the end is inclusive
         */
        template<typename T>
        struct InclusiveMemoryRange
        {
            /**
             * Construct a memory range
             * 
             * @param aBegin The start of the range
             * @param aEnd The end of the range (inclusive)
             */
            constexpr InclusiveMemoryRange(T aBegin, T aEnd)
                : Begin{ aBegin }
                , End{ aEnd }
            {
                if (aBegin > aEnd)
                {
                    PanicNoMMU("Begin should be before or equal to end");
                }
            }

            T Begin;
            T End;
        };

        /**
         * Inserts an instruction barrier which ensures that all instructions following this respect any MMU
         * changes
         */
        void InstructionBarrier()
        {
            // #TODO: Should probably be moved to a common location once we know who else might care (MemoryManager
            // does, with some additional lines)
            // NOLINTNEXTLINE(hicpp-no-assembler)
            asm volatile(
                "dsb ish\n" // data synchronization barrier to ensure everything is committed
                "isb"       // instruction synchronization barrier to ensure all instructions see the changes
                : // no outputs
                : // no inputs
                : // no bashed registers
            );
        }

        class PageBumpAllocator
        {
        public:
            /**
             * Creates a bump allocator for pages, given the range of memory to pull from
             * 
             * @param aRange The range of memory to allocate from
             */
            explicit PageBumpAllocator(ExclusiveMemoryRange<PhysicalPtr> const aRange)
                : Start{ aRange.Begin }
                , End{ aRange.End }
                , Current{ aRange.Begin }
            {
                if (Start > End)
                {
                    PanicNoMMU("Bump allocator start is past the end");
                }
                if (Start.GetAddress() % MemoryManager::PageSize != 0)
                {
                    PanicNoMMU("Bump allocator start address is not aligned to a page size");
                }
                if (End.GetAddress() % MemoryManager::PageSize != 0)
                {
                    PanicNoMMU("Bump allocator start address is not aligned to a page size");
                }
            }

            /**
             * Allocates a single page of memory
             * 
             * @return The allocated page, zeroed out
             */
            PhysicalPtr Allocate()
            {
                if (Current >= End)
                {
                    PanicNoMMU("Bump allocator out of memory");
                }

                auto const retPage = Current;
                Current = Current.Offset(MemoryManager::PageSize);

                // no MMU yet, so physical addresses are real pointers at this stage
                std::memset(std::bit_cast<void*>(retPage.GetAddress()), 0, MemoryManager::PageSize);
                return retPage;
            }

        private:
            PhysicalPtr Start;
            PhysicalPtr End;
            PhysicalPtr Current;
        };

        /**
         * Gets or inserts a page table descriptor into the view for the given virtual address
         * 
         * @param arAllocator Allocator to use for getting new pages
         * @param aTableView The table to check for the descriptor, or to insert a new descriptor into
         * @param aVirtualAddress The virtual address the table will cover
         */
        template<class ChildTableT, class PageTableT>
        ChildTableT GetOrInsertPageDescriptor(PageBumpAllocator& arAllocator, PageTableT const aTableView,
            VirtualPtr const aVirtualAddress)
        {
            auto const entry = aTableView.GetEntryForVA(aVirtualAddress);
            Descriptor::Table descriptor;
            entry.Visit(Overloaded{
                [&descriptor, &arAllocator, &aTableView, aVirtualAddress](Descriptor::Fault)
                {
                    descriptor.Address(arAllocator.Allocate());
                    aTableView.SetEntryForVA(aVirtualAddress, descriptor);
                },
                [&descriptor](Descriptor::Table aTable)
                {
                    descriptor = aTable;
                },
                [](Descriptor::L1Block)
                {
                    PanicNoMMU("Should not have level 1 blocks in boot tables");
                },
                [](Descriptor::L2Block)
                {
                    PanicNoMMU("Should not have level 2 blocks in boot tables");
                }
            });

            // no MMU, so physical address is the pointer
            return ChildTableT{ std::bit_cast<uint64_t*>( descriptor.Address().GetAddress() ) };
        }

        /**
         * Inserts all the required data into the page tables for a level 3 table (which points at 4kb pages)
         * covering the given virtual address.
         * 
         * @param arAllocator The allocator to use for getting new pages
         * @param aRootPage The root level 0 table
         * @param aVirtualAddress The virtual address we want to map and need a table for
         * 
         * @return The level 3 table covering the given virtual address (making it if necessary)
         */
        PageTable::Level3View InsertPageTable(PageBumpAllocator& arAllocator, PageTable::Level0View const aRootPage, VirtualPtr const aVirtualAddress)
        {
            // #TODO: We're assuming level 1 and 2 tables only contain pages and not blocks, which is going to be
            // the case for how our code is currently written. Would be safer to have code that can handle the type
            // of descriptor based on which table is being read.

            // Level 0 table points to level 1 table (512GB range)
            auto const level1Table = GetOrInsertPageDescriptor<PageTable::Level1View>(arAllocator, aRootPage, aVirtualAddress);

            // Level 1 table points to level 2 table (1GB range)
            auto const level2Table = GetOrInsertPageDescriptor<PageTable::Level2View>(arAllocator, level1Table, aVirtualAddress);

            // Level 2 table points to level 3 table (2MB range)
            return GetOrInsertPageDescriptor<PageTable::Level3View>(arAllocator, level2Table, aVirtualAddress);
        }

        /**
         * Inserts entries into the page table to map the given virtual address to the memory block starting at the
         * given physical address, with the given flags.
         * 
         * @param arAllocator Allocator for memory pages
         * @param aRootPage The root page table
         * @param aVARange The virtual address range to map
         * @param aPhysicalAddress The physical address to map to
         * @param aMAIRIndex The index into the MAIR register for the attributes for this memory
         */
        void InsertEntriesForMemoryRange(PageBumpAllocator& arAllocator, PageTable::Level0View const aRootPage,
            InclusiveMemoryRange<VirtualPtr> const aVARange, PhysicalPtr const aPhysicalAddress,
            uint8_t const aMAIRIndex)
        {
            auto curPA = aPhysicalAddress;
            for (auto curVA = aVARange.Begin; curVA < aVARange.End;)
            {
                // Level 3 table points to 4kb blocks
                auto const level3Table = InsertPageTable(arAllocator, aRootPage, curVA);

                Descriptor::Page pageEntry;
                pageEntry.Address(curPA);
                pageEntry.AF(true); // don't fault when accessed
                pageEntry.AP(Descriptor::Page::AccessPermissions::KernelRWUserNone); // only kernel can access
                pageEntry.AttrIndx(aMAIRIndex);

                level3Table.SetEntryForVA(curVA, pageEntry);

                curVA = curVA.Offset(MemoryManager::PageSize);
                curPA = curPA.Offset(MemoryManager::PageSize);
            }
        }

        /**
         * Sets up an identity mapping for the first 1GB of memory in the given level 0 table
         * 
         * @param arAllocator Allocator for memory pages
         * @param aRootPage The root page table
         * @param aMAIRIndex The index into the MAIR register for the attributes for this memory
         */
        void InsertEntriesForIdentityMapping(PageBumpAllocator& arAllocator, PageTable::Level0View const aRootPage,
            uint8_t const aMAIRIndex)
        {
            // Level 0 table points to level 1 table (512GB range)
            auto const level1Table = GetOrInsertPageDescriptor<PageTable::Level1View>(arAllocator, aRootPage, VirtualPtr{ 0 });
            
            Descriptor::L1Block blockEntry;
            blockEntry.Address(PhysicalPtr{ 0 });
            blockEntry.AF(true); // don't fault when accessed
            blockEntry.AP(Descriptor::L1Block::AccessPermissions::KernelRWUserNone); // only kernel can access
            blockEntry.AttrIndx(aMAIRIndex);

            level1Table.SetEntryForVA(VirtualPtr{ 0 }, blockEntry);
        }

        struct PageRoots
        {
            PhysicalPtr Table0; // table for user space (0x0000'0000'0000'0000 - 0x0000'FFFF'FFFF'FFFF)
            PhysicalPtr Table1; // table for kernel space (0xFFFF'0000'0000'0000 - 0xFFFF'FFFF'FFFF'FFFF)
        };

        /**
         * Allocate the page tables, initialize them with enough data to boot, and return the pointers
         * 
         * @return The two tables that were set up
         */
        PageRoots AllocateAndInitPageTables()
        {
            auto const pageDirectoryBuffer = PhysicalPtr{ std::bit_cast<uintptr_t>(&GlobalNoMMU(_pg_dir)) };
            auto const pageDirectoryBufferEnd = PhysicalPtr{ std::bit_cast<uintptr_t>(&GlobalNoMMU(_pg_dir_end)) };
            PageBumpAllocator allocator{ ExclusiveMemoryRange{ pageDirectoryBuffer, pageDirectoryBufferEnd } };

            // #TODO: This is hardcoded for now and we should likely have the individual devices request the addresses
            // they need based on device tree information
            auto const deviceBasePA = MemoryManager::DeviceBaseAddress;
            auto const deviceEndPA = deviceBasePA.Offset(0x00FF'FFFF);

            // Calculate the range of the kernel image in page size size
            auto const kernelBasePA = MemoryManager::CalculateBlockStart(
                PhysicalPtr{ std::bit_cast<uintptr_t>(&GlobalNoMMU(_kernel_image)) }, MemoryManager::PageSize
            );
            auto const kernelEndPA = MemoryManager::CalculateBlockEnd(
                PhysicalPtr{ std::bit_cast<uintptr_t>(&GlobalNoMMU(_kernel_image_end)) }, MemoryManager::PageSize
            );

            // Converts a physical pointer to a virtual pointer assuming offset mapping
            auto toVAOffsetMapping = [](PhysicalPtr const aPA)
            {
                return VirtualPtr{ aPA.GetAddress() }.Offset(MemoryManager::KernelVirtualAddressOffset);
            };

            auto const kernelRangeVA = InclusiveMemoryRange{
                toVAOffsetMapping(kernelBasePA),
                toVAOffsetMapping(kernelEndPA)
            };
            auto const deviceRangeVA = InclusiveMemoryRange{
                toVAOffsetMapping(deviceBasePA),
                toVAOffsetMapping(deviceEndPA)
            };

            // physical addresses that the allocator returns are pointers since we have no MMU at this point
            auto const table1Root = allocator.Allocate();
            auto const table1RootView = PageTable::Level0View{ std::bit_cast<uint64_t*>(table1Root.GetAddress()) };

            // Map the kernel and devices into high memory
            // #TODO: Want to be able to remove the device mapping eventually - once we have a device driver that can map
            InsertEntriesForMemoryRange(allocator, table1RootView, kernelRangeVA, kernelBasePA, MemoryManager::NormalMAIRIndex);
            InsertEntriesForMemoryRange(allocator, table1RootView, deviceRangeVA, deviceBasePA, MemoryManager::DeviceMAIRIndex);

            // Map everything between the kernel range and device range for now
            // #TODO: Should be able to remove this once the memory manager can scan the list of valid addresses from
            // the device tree and map all physical memory into kernel space. Without this, any attempt to allocate
            // pages in MemoryManager.cpp would fail because the memory the page is taken from wouldn't be mapped into
            // kernel space
            auto const extraRangeVA = InclusiveMemoryRange{
                kernelRangeVA.End.Offset(1),
                VirtualPtr{ deviceRangeVA.Begin.GetAddress() - 1 }
            };
            auto const startOfExtraPA = kernelEndPA.Offset(1);
            InsertEntriesForMemoryRange(allocator, table1RootView, extraRangeVA, startOfExtraPA, MemoryManager::NormalMAIRIndex);

            // Now we are going to set up a very basic 1GB block for identity mapping to ensure that when we turn the
            // MMU on we don't immediately trap because the IP and SP are pointing in the lower half (until we
            // update them)
            auto const table0Root = allocator.Allocate();
            auto const table0RootView = PageTable::Level0View{ std::bit_cast<uint64_t*>(table0Root.GetAddress()) };
            InsertEntriesForIdentityMapping(allocator, table0RootView, MemoryManager::NormalMAIRIndex);

            return PageRoots{
                .Table0 = table0Root,
                .Table1 = table1Root
            };
        }

        /**
         * Sets the page table registers
         * 
         * @param aTables The tables to use
         */
        void SwitchToPageTables(PageRoots const aTables)
        {
            TTBRn_EL1 ttbrn_el1;

            ttbrn_el1.BADDR(aTables.Table0);
            TTBRn_EL1::Write0(ttbrn_el1);

            ttbrn_el1.BADDR(aTables.Table1);
            TTBRn_EL1::Write1(ttbrn_el1);
        }
    }

    void InitPageTablesAndMMU()
    {
        auto const tables = AllocateAndInitPageTables();

        SwitchToPageTables(tables);

        MAIR_EL1 mair_el1;
        mair_el1.SetAttribute(MemoryManager::DeviceMAIRIndex, MAIR_EL1::Attribute::DeviceMemory());
        mair_el1.SetAttribute(MemoryManager::NormalMAIRIndex, MAIR_EL1::Attribute::NormalMemory());
        MAIR_EL1::Write(mair_el1);

        // IMPORTANT: Do not change granule size or address bits, because we have a lot of constants that depend on
        // these being set to 4kb and 48 bits respectively.
        static constexpr uint64_t LowAddressBits = 48;
        static constexpr uint64_t FourKB = 0x1000;
        static_assert((~((1ULL << LowAddressBits) - 1)) == MemoryManager::KernelVirtualAddressOffset, "Bit count doesn't match VA start");
        // *4 because we have four tables in our MMU setup
        static_assert(LowAddressBits == PageTable::PageOffsetBits + (PageTable::TableIndexBits * 4), "Bit count doesn't match descriptor bit count");
        static_assert(MemoryManager::PageSize == FourKB, "Expect page size to be 4kb");

        TCR_EL1 tcr_el1;
        // user space will have 48 bits of address space, with 4kb granule
        tcr_el1.T0SZ(LowAddressBits);
        tcr_el1.TG0(TCR_EL1::T0Granule::Size4kb);
        // kernel space will have 48 bits of address space, with 4kb granule
        tcr_el1.T1SZ(LowAddressBits);
        tcr_el1.TG1(TCR_EL1::T1Granule::Size4kb);
        
        TCR_EL1::Write(tcr_el1);

        // Make sure the above changes are seen before enabling the MMU
        InstructionBarrier();

        auto sctlr_el1 = SCTLR_EL1::Read();
        sctlr_el1.M(true); // enable MMU
        SCTLR_EL1::Write(sctlr_el1);

        // Make sure the MMU being enabled is seen by anything following this function
        InstructionBarrier();
    }

    VirtualPtr StoreFlattenedDeviceTree(PhysicalPtr const aDeviceTree)
    {
        // MMU isn't on yet, so physical pointers are "real" pointers at this point
        auto const* const pdeviceTree = std::bit_cast<uint8_t const*>(aDeviceTree.GetAddress());

        auto const* pheader = std::bit_cast<DeviceTree::fdt_header const*>(pdeviceTree);
        if (DeviceTree::ValidateMagicAndVersion(*pheader) == DeviceTree::ValidationStatus::Valid)
        {
            if (pheader->totalsize <= DeviceTreeStorageSizeCS)
            {
                auto* destBuffer = static_cast<uint8_t*>(GlobalNoMMU(DeviceTreeStorageS));
                // #TODO: Why can't we use memcpy yet? (complier may be substituting its own copy that uses opcodes
                // that are disabled at this point)
                for (auto curByte = 0U; curByte < pheader->totalsize; ++curByte)
                {
                    // #NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    destBuffer[curByte] = pdeviceTree[curByte];
                }
                
                auto const deviceTreePA = PhysicalPtr{ std::bit_cast<uintptr_t>(destBuffer) };
                return VirtualPtr{
                    deviceTreePA.Offset(CalculatePhysicalToLinkTimeAddressOffset()).GetAddress()
                };
            }
            else
            {
                PanicNoMMU("Device tree too large");
                return VirtualPtr{ 0 };
            }
        }
        else
        {
            PanicNoMMU("Invalid device tree");
            return VirtualPtr{ 0 };
        }
    }

    void UnmapIdentityMapping()
    {
        // Can't use CalculatePhysicalToLinkTimeAddressOffset because the MMU is on and the offset would therefore be
        // 0 (but we could probably store the result of a pre-MMU call of that for later use)
        // #TODO: Maybe calculate the offset pre-MMU and store it instead

        auto const ttbrn_el1 = TTBRn_EL1::Read0();
        auto const rootPagePA = ttbrn_el1.BADDR();
        // The page tables are offset-mapped at this point, so just shift the pointer into kernel space
        auto const rootPageVA = VirtualPtr{ rootPagePA.Offset(MemoryManager::KernelVirtualAddressOffset).GetAddress() };
        auto* const prootPageBuffer = std::bit_cast<uint64_t*>(rootPageVA.GetAddress());

        // manually dig out the level 1 block for our zero address and clear it, which will nuke our identity mapping
        // (though it won't return the pages we used, as we aren't keeping track of that yet)
        // #TODO: See if we can reclaim the memory
        auto const rootPage = PageTable::Level0View{ prootPageBuffer };
        auto identityLevel1Entry = rootPage.GetEntryForVA(VirtualPtr{ 0 });
        PhysicalPtr level1TablePtr;
        identityLevel1Entry.Visit(Overloaded{
            [](auto)
            {
                // if it's not a table, then nothing is mapped here and we do nothing
            },
            [&level1TablePtr](Descriptor::Table const aTable)
            {
                level1TablePtr = aTable.Address();
            }
        });
        
        if (level1TablePtr.GetAddress() != 0)
        {
            auto const level1TableVirtual = VirtualPtr{ level1TablePtr.Offset(MemoryManager::KernelVirtualAddressOffset).GetAddress() };
            auto const level1Table = PageTable::Level1View{ std::bit_cast<uint64_t*>(level1TableVirtual.GetAddress()) };
            level1Table.SetEntryForVA(VirtualPtr{ 0 }, Descriptor::Fault{} );
        }
    }

    uintptr_t AdjustPointerForNoMMU(uintptr_t const aPtr)
    {
        auto const offset = CalculatePhysicalToLinkTimeAddressOffset();
        // Sometimes we have a physical address (usually calculated via PC-relative operations) and sometimes we have a
        // virtual address (usually calculated by the linker). Which one we get can change link-by-line or even based
        // on build configuration, so we have to detect it here and handle it (we can't rely on always getting virtual
        // addresses).
        // #TODO: Would really like to get rid of this conditional and figure out why the pointers are inconsistent
        if (aPtr >= offset)
        {
            // Pointer is a virtual address, so strip out the offset
            return aPtr - offset;
        }
        // Otherwise we assume the pointer is a physical address and return it unchanged
        return aPtr;
    }
}