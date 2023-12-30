// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include <cstdint>
#include <cstring>
#include "../../MemoryManager.h"
#include "../../Utils.h"
#include "../MemoryDescriptor.h"
#include "../SystemRegisters.h"
#include "MMU.h"
#include "Output.h"

// Address translation documentation: https://documentation-service.arm.com/static/5efa1d23dbdee951c1ccdec5?token=

extern "C"
{
    // from link.ld
    extern uint8_t __pg_dir[];
    extern uint8_t __pg_dir_end[]; // past the end
    extern uint8_t __kernel_image[];
    extern uint8_t __kernel_image_end[];
}

namespace AArch64
{
    namespace Boot
    {
        namespace
        {
            /**
             * Inserts an instruction barrier which ensures that all instructions following this respect any MMU
             * changes
            */
            void InstructionBarrier()
            {
                // #TODO: Should probably be moved to a common location once we know who else might care (MemoryManager
                // does, with some additional lines)
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
                 * @param apStart The start of the memory range to allocate from
                 * @param apEnd One past the end of the memory range to allocate from
                */
                PageBumpAllocator(void* const apStart, void* const apEnd)
                    : pStart{apStart}
                    , pEnd{apEnd}
                    , pCurrent{apStart}
                {
                    if (pStart > pEnd)
                    {
                        Panic("Bump allocator start is past the end");
                    }
                    if (reinterpret_cast<uintptr_t>(pStart) % MemoryManager::PageSize != 0)
                    {
                        Panic("Bump allocator start address is not aligned to a page size");
                    }
                    if (reinterpret_cast<uintptr_t>(pEnd) % MemoryManager::PageSize != 0)
                    {
                        Panic("Bump allocator start address is not aligned to a page size");
                    }
                }

                /**
                 * Allocates a single page of memory
                 * 
                 * @return The allocated page, zeroed out
                */
                void* Allocate()
                {
                    if (pCurrent >= pEnd)
                    {
                        Panic("Bump allocator out of memory");
                    }

                    auto const pret = pCurrent;
                    pCurrent = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pCurrent) + MemoryManager::PageSize);

                    std::memset(pret, 0, MemoryManager::PageSize);
                    return pret;
                }

            private:
                void* pStart = nullptr;
                void* pEnd = nullptr;
                void* pCurrent = nullptr;
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
                uint64_t const aVirtualAddress)
            {
                auto const entry = aTableView.GetEntryForVA(aVirtualAddress);
                Descriptor::Table descriptor;
                entry.Visit(Overloaded{
                    [&descriptor, &arAllocator, &aTableView, aVirtualAddress](Descriptor::Fault)
                    {
                        descriptor.Address(reinterpret_cast<uintptr_t>(arAllocator.Allocate()));
                        aTableView.SetEntryForVA(aVirtualAddress, descriptor);
                    },
                    [&descriptor](Descriptor::Table aTable)
                    {
                        descriptor = aTable;
                    },
                    [](Descriptor::L1Block)
                    {
                        Panic("Should not have level 1 blocks in boot tables");
                    },
                    [](Descriptor::L2Block)
                    {
                        Panic("Should not have level 2 blocks in boot tables");
                    }
                });

                return ChildTableT{ reinterpret_cast<uint64_t*>( descriptor.Address() ) };
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
            PageTable::Level3View InsertPageTable(PageBumpAllocator& arAllocator, PageTable::Level0View const aRootPage, uint64_t const aVirtualAddress)
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
             * @param aVAStart The start of the virtual address range to map
             * @param aVAEnd The end of the virtual address range to map
             * @param aPhysicalAddress The physical address to map to
             * @param aMAIRIndex The index into the MAIR register for the attributes for this memory
             */
            void InsertEntriesForMemoryRange(PageBumpAllocator& arAllocator, PageTable::Level0View const aRootPage,
                uint64_t const aVAStart, uint64_t const aVAEnd, uint64_t const aPhysicalAddress,
                uint8_t const aMAIRIndex)
            {
                auto curPA = aPhysicalAddress;
                for (auto curVA = aVAStart; curVA < aVAEnd;)
                {
                    // Level 3 table points to 4kb blocks
                    auto const level3Table = InsertPageTable(arAllocator, aRootPage, curVA);

                    Descriptor::Page pageEntry;
                    pageEntry.Address(curPA);
                    pageEntry.AF(true); // don't fault when accessed
                    pageEntry.AP(Descriptor::Page::AccessPermissions::KernelRWUserNone); // only kernel can access
                    pageEntry.AttrIndx(aMAIRIndex);

                    level3Table.SetEntryForVA(curVA, pageEntry);

                    curVA += MemoryManager::PageSize; 
                    curPA += MemoryManager::PageSize;
                }
            }

            /**
             * Sets the page table registers to the given table
             * 
             * @param apTable The table to use
            */
            void SwitchToPageTable(uint8_t* const apTable)
            {
                // the apTable pointer gets the top 16 bits masked out (because it becomes the ASID), so we don't have
                // to do any adjustment to it to account for it being on a virtual kernel address
                // #TODO: In theory, but the debugger shows it at the physical address for an unknown reason.
                TTBRn_EL1 ttbrn_el1;
                ttbrn_el1.BADDR(reinterpret_cast<uintptr_t>(apTable));
                TTBRn_EL1::Write0(ttbrn_el1); // table for user space (0x0000'0000'0000'0000 - 0x0000'FFFF'FFFF'FFFF)
                TTBRn_EL1::Write1(ttbrn_el1); // table for kernel space (0xFFFF'0000'0000'0000 - 0xFFFF'FFFF'FFFF'FFFF)
            }
        }
    
        void CreatePageTables()
        {
            // #TODO: Currently unclear why the address stored in __pg_dir appears to be the physical address and not
            // the virtual address. But this seems to work for now.
            PageBumpAllocator allocator{ __pg_dir, __pg_dir_end };

            // #TODO: Should make physical and virtual address types to easily differentiate between the two

            // #TODO: This is hardcoded for now and we should likely have the individual devices request the addresses
            // they need based on device tree information
            auto const deviceBasePA = MemoryManager::DeviceBaseAddress;
            auto const deviceEndPA = deviceBasePA.Offset(0x00FF'FFFF);

            // Calculate the range of the kernel image in L2 block size
            // #TODO: Originally this was done so we didn't have to set up 4k pages and could instead use 2MB blocks,
            // but it may not make sense anymore, especially since we later want to flag certain areas as read-only
            // #TODO: Why are these symbols from the linker script pointing at physical addresses?
            auto const kernelBasePA = MemoryManager::CalculateBlockStart(PhysicalPtr{ reinterpret_cast<uintptr_t>(__kernel_image) }, MemoryManager::L2BlockSize);
            auto const kernelEndPA = MemoryManager::CalculateBlockEnd(PhysicalPtr{ reinterpret_cast<uintptr_t>(__kernel_image_end) }, MemoryManager::L2BlockSize);

            // Converts a physical pointer to a virtual pointer assuming identity mapping
            auto toVAIdentityMapping = [](PhysicalPtr const aPA)
            {
                return VirtualPtr{ aPA.GetAddress() }.Offset(MemoryManager::KernelVirtualAddressStart.GetAddress());
            };

            auto const startOfKernelRangeVA = toVAIdentityMapping(kernelBasePA);
            auto const endOfKernelRangeVA = toVAIdentityMapping(kernelEndPA);
            auto const startOfDeviceRangeVA = toVAIdentityMapping(deviceBasePA);
            auto const endOfDeviceRangeVA = toVAIdentityMapping(deviceEndPA);

            auto const rootPage = PageTable::Level0View{ reinterpret_cast<uint64_t*>(allocator.Allocate()) };

            // Identity mappings - so we don't break immediately when turning the MMU on (since the stack and IP will
            // be pointing at the physical addresses)
            InsertEntriesForMemoryRange(allocator, rootPage, kernelBasePA.GetAddress(), kernelEndPA.GetAddress(), kernelBasePA.GetAddress(), MemoryManager::NormalMAIRIndex);
            InsertEntriesForMemoryRange(allocator, rootPage, deviceBasePA.GetAddress(), deviceEndPA.GetAddress(), deviceBasePA.GetAddress(), MemoryManager::DeviceMAIRIndex);

            // Now map the kernel and devices into high memory
            InsertEntriesForMemoryRange(allocator, rootPage, startOfKernelRangeVA.GetAddress(), endOfKernelRangeVA.GetAddress(), kernelBasePA.GetAddress(), MemoryManager::NormalMAIRIndex);
            InsertEntriesForMemoryRange(allocator, rootPage, startOfDeviceRangeVA.GetAddress(), endOfDeviceRangeVA.GetAddress(), deviceBasePA.GetAddress(), MemoryManager::DeviceMAIRIndex);

            // Map everything between the kernel range and device range for now
            // #TODO: Should be able to remove this once the memory manager can scan the list of valid addresses from
            // the device tree and map all physical memory into kernel space. Without this, any attempt to allocate
            // pages in MemoryManager.cpp would fail because the memory the page is taken from wouldn't be mapped into
            // kernel space
            InsertEntriesForMemoryRange(allocator, rootPage, endOfKernelRangeVA.GetAddress() + 1, startOfDeviceRangeVA.GetAddress() - 1, endOfKernelRangeVA.GetAddress() + 1, MemoryManager::NormalMAIRIndex);
        }

        void EnableMMU()
        {
            SwitchToPageTable(__pg_dir);

            MAIR_EL1 mair_el1;
            mair_el1.SetAttribute(MemoryManager::DeviceMAIRIndex, MAIR_EL1::Attribute::DeviceMemory());
            mair_el1.SetAttribute(MemoryManager::NormalMAIRIndex, MAIR_EL1::Attribute::NormalMemory());
            MAIR_EL1::Write(mair_el1);

            // IMPORTANT: Do not change granule size or address bits, because we have a lot of constants that depend on
            // these being set to 4kb and 48 bits respectively.
            static constexpr uint64_t LowAddressBits = 48;
            static_assert((~((1ULL << LowAddressBits) - 1)) == MemoryManager::KernelVirtualAddressStart.GetAddress(), "Bit count doesn't match VA start");
            // *4 because we have four tables in our MMU setup
            static_assert(LowAddressBits == PageTable::PageOffsetBits + (PageTable::TableIndexBits * 4), "Bit count doesn't match descriptor bit count");
            static_assert(MemoryManager::PageSize == 0x1000, "Expect page size to be 4kb");

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
    }
}