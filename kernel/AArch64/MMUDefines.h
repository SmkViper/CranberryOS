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

// How large a page is (4k)
#define PAGE_SIZE (1 << PAGE_SHIFT)
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
// Page Descriptor Layout:
// +------------------+---------+------------------+-------------+-------+
// | Upper attributes | Address | Lower attributes | Block/table | Valid |
// +------------------+---------+------------------+-------------+-------+
// 63                 47        11                 2             1       0
//
// Upper attributes - a set of attributes
// Address - the actual address of the next page table or physical page/section. Since these are always page aligned,
// the lower 12 bits of the address are always zero and can be re-used for other things.
// Lower attributes - a set of attributes for block descriptors (ignored for table descriptors)
// Block/table - a single bit that, if set, indicates that this points at a table instead of a block of memory
// Valid - a single bit that flags whether what this points at is valid or not. If this is not set, a translation fault
//   is generated
/////////////////////////////////////////////////

// 0b11 valid table descriptor
#define MM_TYPE_PAGE_TABLE 0x3
#define MM_TYPE_PAGE 0x3
// 0b01 valid block descriptor
#define MM_TYPE_BLOCK 0x1
// Single bit flag that, when 1, means we won't generate an access flag fault when the memory is accessed
#define MM_ACCESS (0x1 << 10)
// 0b01 specifying read/write access from EL0 and higher
#define MM_ACCESS_PERMISSION (0x1 << 6)

// MAIR register values/indices so the MMU can compress flags from 8 bytes to 3 by indexing the 64-bit MAIR register
// which is essentially in 8-bit chunks.

/////////////////////////////////////////////////
// Indicies into the MAIR register.
// These select which 8-bit chunk of flags to use
/////////////////////////////////////////////////

// Flags a page as accessing device memory
#define MT_DEVICE_nGnRnE 0x0
// Flags a page as normal non-cacheable memory
#define MT_NORMAL_NC 0x1

/////////////////////////////////////////////////
// Descriptor flags for various descriptor types
/////////////////////////////////////////////////

// Flags for a descriptor that points at a block of normal non-cacheable memory
#define MMU_FLAGS (MM_TYPE_BLOCK | (MT_NORMAL_NC << 2) | MM_ACCESS)
// Flags for a descriptor that points at a block of device nGnRnE memory
#define MMU_DEVICE_FLAGS (MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)
// Flags for a descriptor that points at a page
// #TODO: Why is NORMAL_NC, ACCESS, and ACCESS_PERMISSION specified? Low flags I thought were ignored for page table descriptors
#define MMU_PTE_FLAGS (MM_TYPE_PAGE | (MT_NORMAL_NC << 2) | MM_ACCESS | MM_ACCESS_PERMISSION)

#endif // KERNEL_AARCH64_MMU_DEFINES_H