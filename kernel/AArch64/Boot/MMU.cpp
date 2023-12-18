// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include <cstdint>
#include <cstring>
#include "../MMUDefines.h"
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
                // #TODO: Should probably be moved to a common location once we know who else might care
                asm volatile(
                    "isb"
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
                    if (reinterpret_cast<uintptr_t>(pStart) % PAGE_SIZE != 0)
                    {
                        Panic("Bump allocator start address is not aligned to a page size");
                    }
                    if (reinterpret_cast<uintptr_t>(pEnd) % PAGE_SIZE != 0)
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
                    pCurrent = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pCurrent) + PAGE_SIZE);

                    std::memset(pret, 0, PAGE_SIZE);
                    return pret;
                }

            private:
                void* pStart = nullptr;
                void* pEnd = nullptr;
                void* pCurrent = nullptr;
            };

            /**
             * Converts a page table descriptor to a pointer to the next page table
             * 
             * @param aDescriptor The descriptor to convert
             * 
             * @return The page table the descriptor points at
            */
            uint64_t* DescriptorToPointer(uint64_t const aDescriptor)
            {
                // #TODO: The descriptor probably should just be its own type
                auto const DescriptorMask = ~0b11;
                return reinterpret_cast<uint64_t*>(aDescriptor & DescriptorMask);
            }

            /**
             * Inserts all the required data into the page tables for a level 3 table (which points at 4kb pages)
             * covering the given virtual address.
             * 
             * @param arAllocator The allocator to use for getting new pages
             * @param apRootPage The root level 0 table
             * @param aVirtualAddress The virtual address we want to map and need a table for
             * 
             * @return The level 3 table covering the given virtual address (making it if necessary)
            */
            uint64_t* InsertPageTable(PageBumpAllocator& arAllocator, uint64_t* const apRootPage, uint64_t const aVirtualAddress)
            {
                // #TODO: Can probably be cleaned up to reduce rudundancy

                // Level 0 table points to level 1 table (512GB range)
                auto const level1TableIndex = (aVirtualAddress >> PGD_SHIFT) & (PTRS_PER_TABLE - 1);
                if (apRootPage[level1TableIndex] == 0)
                {
                    apRootPage[level1TableIndex] = reinterpret_cast<uint64_t>(arAllocator.Allocate());
                    apRootPage[level1TableIndex] |= MM_TYPE_PAGE_TABLE;
                }

                // Level 1 table points to level 2 table (1GB range)
                auto const plevel1Table = DescriptorToPointer(apRootPage[level1TableIndex]);
                auto const level2TableIndex = (aVirtualAddress >> PUD_SHIFT) & (PTRS_PER_TABLE - 1);
                if (plevel1Table[level2TableIndex] == 0)
                {
                    plevel1Table[level2TableIndex] = reinterpret_cast<uint64_t>(arAllocator.Allocate());
                    plevel1Table[level2TableIndex] |= MM_TYPE_PAGE_TABLE;
                }

                // Level 2 table points to level 3 table (2MB range)
                auto const plevel2Table = DescriptorToPointer(plevel1Table[level2TableIndex]);
                auto const level3TableIndex = (aVirtualAddress >> PMD_SHIFT) & (PTRS_PER_TABLE - 1);
                if (plevel2Table[level3TableIndex] == 0)
                {
                    plevel2Table[level3TableIndex] = reinterpret_cast<uint64_t>(arAllocator.Allocate());
                    plevel2Table[level3TableIndex] |= MM_TYPE_PAGE_TABLE;
                }

                return DescriptorToPointer(plevel2Table[level3TableIndex]);
            }

            /**
             * Inserts entries into the page table to map the given virtual address to the memory block starting at the
             * given physical address, with the given flags.
             * 
             * @param arAllocator Allocator for memory pages
             * @param apRootPage The root page table
             * @param aVAStart The start of the virtual address range to map
             * @param aVAEnd The end of the virtual address range to map
             * @param aPhysicalAddress The physical address to map to
             * @param aFlags The flags for the new entries
            */
            void InsertEntriesForMemoryRange(PageBumpAllocator& arAllocator, uint64_t* const apRootPage, uint64_t const aVAStart, uint64_t const aVAEnd,
                uint64_t const aPhysicalAddress, uint64_t const aFlags)
            {
                auto curPA = aPhysicalAddress;
                for (auto curVA = aVAStart; curVA < aVAEnd;)
                {
                    // Level 3 table points to 4kb blocks
                    auto const plevel3Table = InsertPageTable(arAllocator, apRootPage, curVA);

                    auto const blockIndex = (curVA >> PAGE_SHIFT) & (PTRS_PER_TABLE - 1);
                    auto& rblockEntry = plevel3Table[blockIndex];
                    rblockEntry = curPA;
                    rblockEntry |= aFlags;

                    curVA += PAGE_SIZE; 
                    curPA += PAGE_SIZE;
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
            auto const deviceBasePA = static_cast<uintptr_t>(DEVICE_BASE);
            auto const deviceEndPA = static_cast<uintptr_t>(deviceBasePA + 0x00FF'FFFF);

            // Calculate the range of the kernel image in 2MB blocks (since 2MB is the size of the blocks pointed at by
            // the level 2 table since we're running with 4KB granule)
            // #TODO: Why are these symbols from the linker script pointing at physical addresses?
            auto const kernelBasePA = reinterpret_cast<uintptr_t>(__kernel_image) & (~static_cast<uintptr_t>(0x001F'FFFF));
            auto const kernelEndPA = (reinterpret_cast<uintptr_t>(__kernel_image_end) & (~static_cast<uintptr_t>(0x001F'FFFF)) + 0x0020'0000 - 1);

            auto const startOfKernelRangeVA = kernelBasePA + VA_START;
            auto const endOfKernelRangeVA = kernelEndPA + VA_START;
            auto const startOfDeviceRangeVA = deviceBasePA + VA_START;
            auto const endOfDeviceRangeVA = deviceEndPA + VA_START;

            auto const prootPage = reinterpret_cast<uint64_t*>(allocator.Allocate());

            // #TODO: Should really be a custom type
            auto const normalMemory = MM_ACCESS | MM_TYPE_PAGE | (MT_NORMAL_NC << 2);
            auto const deviceMemory = MM_ACCESS | MM_TYPE_PAGE | (MT_DEVICE_nGnRnE << 2);

            // Identity mappings - so we don't break immediately when turning the MMU on (since the stack and IP will
            // be pointing at the physical addresses)
            InsertEntriesForMemoryRange(allocator, prootPage, kernelBasePA, kernelEndPA, kernelBasePA, normalMemory);
            InsertEntriesForMemoryRange(allocator, prootPage, deviceBasePA, deviceEndPA, deviceBasePA, deviceMemory);

            // Now map the kernel and devices into high memory
            InsertEntriesForMemoryRange(allocator, prootPage, startOfKernelRangeVA, endOfKernelRangeVA, kernelBasePA, normalMemory);
            InsertEntriesForMemoryRange(allocator, prootPage, startOfDeviceRangeVA, endOfDeviceRangeVA, deviceBasePA, deviceMemory);

            // Map everything between the kernel range and device range for now
            // #TODO: Should be able to remove this once the memory manager can scan the list of valid addresses from
            // the device tree and map all physical memory into kernel space. Without this, any attempt to allocate
            // pages in MemoryManager.cpp would fail because the memory the page is taken from wouldn't be mapped into
            // kernel space
            InsertEntriesForMemoryRange(allocator, prootPage, endOfKernelRangeVA + 1, startOfDeviceRangeVA - 1, endOfKernelRangeVA + 1, normalMemory);
        }

        void EnableMMU()
        {
            SwitchToPageTable(__pg_dir);

            MAIR_EL1 mair_el1;
            mair_el1.SetAttribute(MT_DEVICE_nGnRnE, MAIR_EL1::Attribute::DeviceMemory());
            mair_el1.SetAttribute(MT_NORMAL_NC, MAIR_EL1::Attribute::NormalMemory());
            MAIR_EL1::Write(mair_el1);

            TCR_EL1 tcr_el1;
            // user space will have 48 bits of address space, with 4kb granule
            tcr_el1.T0SZ(48);
            tcr_el1.TG0(TCR_EL1::T0Granule::Size4kb);
            // kernel space will have 48 bits of address space, with 4kb granule
            // #TODO: Sync with VA_START somehow?
            tcr_el1.T1SZ(48);
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