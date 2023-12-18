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
             * Sets up a page table entry for the given virtual address.
             * 
             * @param apTable The table to put the entry into
             * @param apNextTable The next table in the chain for the virtual address
             * @param aTargetVirtualAddress The target virtual address to map
             * @param aIndexShift Shift that needs to be applied to the virtual address in order to get the current
             * table index
            */
            void CreateTableEntry(uintptr_t* const apTable, uintptr_t* const apNextTable, uintptr_t const aTargetVirtualAddress, uint8_t const aIndexShift)
            {
                // shift the address to strip off anything to the right of the table index, then AND it with the
                // maximum index to strip off anything to the left, ending up with the index this table is for
                uintptr_t const index = (aTargetVirtualAddress >> aIndexShift) & (PTRS_PER_TABLE - 1);

                // flag the descriptor as pointing at a table, and being valid
                uint64_t const nextPageEntry = reinterpret_cast<uintptr_t>(apNextTable) | MM_TYPE_PAGE_TABLE;
                
                // store the descriptor into the table at the index
                apTable[index] = nextPageEntry;
            }

            /**
             * Fills out the page middle directory
             * 
             * @param apTableStart Pointer to the PMD being filled
             * @param aTargetPhysicalRegion Start of the physical region being mapped
             * @param aVirtualStart Virtual address of the first section being mapped
             * @param aVirtualEnd Virtual address of the last section being mapped
             * @param aFlags The flags to be put into the descriptor
            */
            void CreateBlockMap(uint64_t* const apTableStart, uintptr_t const aTargetPhysicalRegion, uintptr_t const aVirtualStart, uintptr_t const aVirtualEnd, uintptr_t const aFlags)
            {
                auto const startIndex = (aVirtualStart >> SECTION_SHIFT) & (PTRS_PER_TABLE - 1);
                auto const endIndex = (aVirtualEnd >> SECTION_SHIFT) & (PTRS_PER_TABLE - 1);

                // Double-shifting to trim off the low bits and apply the flags in the now empty space to form the
                // block descriptor
                auto curBlockDescriptor = ((aTargetPhysicalRegion >> SECTION_SHIFT) << SECTION_SHIFT) | aFlags;

                // #TODO: Should really just make endIndex be one-past-the-end, then caller doesn't need to do
                // -SECTION_SIZE
                for (auto curIndex = startIndex; curIndex <= endIndex; ++curIndex)
                {
                    apTableStart[curIndex] = curBlockDescriptor;
                    curBlockDescriptor += SECTION_SIZE;
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

            // #TODO: This is all rather hard-coded and should be made more flexible based off of external information
            // like the actual kernel size.

            // allocate the three pages we need for the mapping we're doing
            auto const ppageGlobalDirectory = reinterpret_cast<uintptr_t*>(allocator.Allocate());
            auto const ppageUpperDirectory = reinterpret_cast<uintptr_t*>(allocator.Allocate());
            auto const ppageMiddleDirectory = reinterpret_cast<uintptr_t*>(allocator.Allocate());

            CreateTableEntry(ppageGlobalDirectory, ppageUpperDirectory, VA_START, PGD_SHIFT);
            CreateTableEntry(ppageUpperDirectory, ppageMiddleDirectory, VA_START, PUD_SHIFT);

            // #TODO: We should be making sure we only map what we need, not "everything". And definitely not
            // hardcoding how much physical memory we have. That should be left up to the real memory manager which
            // will know the memory information from the device tree.

            // Map all memory from 0 up to DEVICE_BASE as regular memory (- SECTION_SIZE because the function expects
            // the end value to be the address of the last section to map)
            CreateBlockMap(ppageMiddleDirectory, 0, VA_START, (VA_START + DEVICE_BASE - SECTION_SIZE), MMU_FLAGS);
            // Map all memory from the DEVICE_BASE up to the end of physical memory as MMIO memory
            CreateBlockMap(ppageMiddleDirectory, DEVICE_BASE, (VA_START + DEVICE_BASE), (VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE), MMU_DEVICE_FLAGS);
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