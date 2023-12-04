// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include <cstdint>
#include <cstring>
#include "../MMUDefines.h"
#include "Output.h"

extern "C"
{
    extern uint8_t __pg_dir[];
}

namespace AArch64
{
    namespace Boot
    {
        // #TODO: Really should consolidate all these ASM calls in one location for re-use
        namespace ASM
        {
            /**
             * Sets the ttbr0_el1 register to the given value. This sets up the translation table for stage 1
             * translation from the lower virtual address range (upper bits all 0) in EL0 and 1.
             * See: https://developer.arm.com/documentation/ddi0595/2021-09/AArch64-Registers/TTBR0-EL1--Translation-Table-Base-Register-0--EL1-
             * 
             * @param aValue The value to set to.
            */
            void SetTTBR0_EL1(uintptr_t const aValue)
            {
                asm volatile(
                    "msr ttbr0_el1, %[value]"
                    : // no output
                    : [value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }

            /**
             * Sets the ttbr1_el1 register to the given value. This sets up the translation table for stage 1
             * translation from the higher virtual address range (upper bits all 1) in EL0 and 1.
             * See: https://developer.arm.com/documentation/ddi0595/2021-09/AArch64-Registers/TTBR1-EL1--Translation-Table-Base-Register-1--EL1-
             * 
             * @param aValue The value to set to.
            */
            void SetTTBR1_EL1(uintptr_t const aValue)
            {
                // #TODO: Would be nice to have a bitfield value that was type safe to pass in
                asm volatile(
                    "msr ttbr1_el1, %[value]"
                    : // no output
                    : [value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }

            /**
             * Sets the mair_el1 register to the given value. This is the memory attribute encodings for index values
             * in the translation table.
             * See: https://developer.arm.com/documentation/ddi0595/2020-12/AArch64-Registers/MAIR-EL1--Memory-Attribute-Indirection-Register--EL1-
             * 
             * @param aValue The value to set to.
            */
            void SetMAIR_EL1(uint64_t const aValue)
            {
                // #TODO: Would be nice to have a bitfield value that was type safe to pass in
                asm volatile(
                    "msr mair_el1, %[value]"
                    : // no outputs
                    : [value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }

            /**
             * Sets the tcr_el register to the given value. This controls stage 1 of the EL1 and 0 translation regime.
             * See: https://developer.arm.com/documentation/ddi0595/2021-09/AArch64-Registers/TCR-EL1--Translation-Control-Register--EL1-
             * 
             * @param aValue The value to set to.
            */
            void SetTCR_EL1(uint64_t const aValue)
            {
                // #TODO: Would be nice to have a bitfield value that was type safe to pass in
                asm volatile(
                    "msr tcr_el1, %[value]"
                    : // no outputs
                    : [value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }
        }

        namespace
        {
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
                ASM::SetTTBR0_EL1(reinterpret_cast<uintptr_t>(apTable));
                ASM::SetTTBR1_EL1(reinterpret_cast<uintptr_t>(apTable));
            }
        }
    }
}

extern "C"
{
    /**
     * Called from assembly to create our boot page tables
    */
    void create_page_tables()
    {
        // #TODO: Currently unclear why the address stored in __pg_dir appears to be the physical address and not the
        // virtual address. But this seems to work for now.
        AArch64::Boot::PageBumpAllocator allocator{__pg_dir, __pg_dir + PG_DIR_SIZE};

        // #TODO: This is all rather hard-coded and should be made more flexible based off of external information like
        // the actual kernel size.

        // allocate the three pages we need for the mapping we're doing
        auto const ppageGlobalDirectory = reinterpret_cast<uintptr_t*>(allocator.Allocate());
        auto const ppageUpperDirectory = reinterpret_cast<uintptr_t*>(allocator.Allocate());
        auto const ppageMiddleDirectory = reinterpret_cast<uintptr_t*>(allocator.Allocate());

        AArch64::Boot::CreateTableEntry(ppageGlobalDirectory, ppageUpperDirectory, VA_START, PGD_SHIFT);
        AArch64::Boot::CreateTableEntry(ppageUpperDirectory, ppageMiddleDirectory, VA_START, PUD_SHIFT);

        // #TODO: We should be making sure we only map what we need, not "everything". And definitely not hardcoding
        // how much physical memory we have. That should be left up to the real memory manager which will know the
        // memory information from the device tree.

        // Map all memory from 0 up to DEVICE_BASE as regular memory (- SECTION_SIZE because the function expects the
        // end value to be the address of the last section to map)
        AArch64::Boot::CreateBlockMap(ppageMiddleDirectory, 0, VA_START, (VA_START + DEVICE_BASE - SECTION_SIZE), MMU_FLAGS);
        // Map all memory from the DEVICE_BASE up to the end of physical memory as MMIO memory
        AArch64::Boot::CreateBlockMap(ppageMiddleDirectory, DEVICE_BASE, (VA_START + DEVICE_BASE), (VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE), MMU_DEVICE_FLAGS);
    }

    /**
     * Called from assembly to turn on the MMU
    */
    void enable_mmu()
    {
        // #TODO: Not sure why the __pg_dir pointer doesn't need to be adjusted.
        AArch64::Boot::SwitchToPageTable(__pg_dir);

        // #TODO: Should make some nice type-safe wrappers for the register values
        AArch64::Boot::ASM::SetMAIR_EL1(MAIR_VALUE);
        AArch64::Boot::ASM::SetTCR_EL1(TCR_VALUE);

        // #TODO: Enable the MMU
    }
}