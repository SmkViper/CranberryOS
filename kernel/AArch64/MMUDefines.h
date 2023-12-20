// Included from assembly, so can't have anything fancy in here

#ifndef KERNEL_AARCH64_MMU_DEFINES_H
#define KERNEL_AARCH64_MMU_DEFINES_H

// #TODO: Clean all this up in some way so we don't have so many #defines polluting the global namespace

/////////////////////////////////////////////////
// Virtual address layout:
// +------+-----------+-----------+-----------+-----------+-------------+
// |      | PGD Index | PUD Index | PMD Index | PTE Index | Page offset |
// +------+-----------+-----------+-----------+-----------+-------------+
// 63     47          38          29          20          11            0
//
// PGD Index - index into the Page Global Directory
// PUD Index - index into the Page Upper Directory
// PMD Index - index into the Page Middle Directory
// PTE Index - index into the Page Table Directory
// Page offset - offset of the physical address from the start of the page pointed at by the PTE entry
//
// For section mapping, the PTE Index is omitted, and bits 20:0 are used instead to offset into the 2mb section pointed
// at by the PMD entry
/////////////////////////////////////////////////

// Number of bits in the virtual address representing the offset into a 4k page (bits 11:0)
#define PAGE_SHIFT 12
// Number of bits in the virtual address representing the table index (9 bits each)
#define TABLE_SHIFT 9
// Number of bits in the virtual address representing the offset into a 2mb section (bits 20:0)
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

// How large a section is (2mb)
#define SECTION_SIZE (1 << SECTION_SHIFT)

// Reserve "low memory" for the kernel (4mb)
#define LOW_MEMORY (2 * SECTION_SIZE)

// How many pointers can fit into a single table
#define PTRS_PER_TABLE (1 << TABLE_SHIFT)

// How far to shift the virtual address to get the Page Global Directory index
#define PGD_SHIFT (PAGE_SHIFT + 3*TABLE_SHIFT)
// How far to shift the virtual address to get the Page Upper Directory index
#define PUD_SHIFT (PAGE_SHIFT + 2*TABLE_SHIFT)
// How far to shift the virtual address to get the Page Middle Directory index
#define PMD_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

/////////////////////////////////////////////////
// Indicies into the MAIR register.
// These select which 8-bit chunk of flags to use
/////////////////////////////////////////////////

// Flags a page as accessing device memory
#define MT_DEVICE_nGnRnE 0x0
// Flags a page as normal non-cacheable memory
#define MT_NORMAL_NC 0x1

#endif // KERNEL_AARCH64_MMU_DEFINES_H