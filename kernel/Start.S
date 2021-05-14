#include "ARM/MMUDefines.h"
#include "ARM/SystemRegisters.h"

// Macro to switch the current processor to el1, using the given scratch
// register. Assumes we are in EL2. If there is some way to boot the RPi into
// EL3, then we'll have to add some additional code to move into EL2 first.
.macro armv8_switch_to_el1, xreg1
    // Initialize the processor settings (see SystemRegisters.h)
    ldr     \xreg1, =SCTLR_INIT_VALUE
    msr     sctlr_el1, \xreg1

    ldr     \xreg1, =HCR_EL2_INIT_VALUE
    msr     hcr_el2, \xreg1

    ldr     \xreg1, =SPSR_EL2_INIT_VALUE
    msr     spsr_el2, \xreg1

    // Disable all traps so that EL2, EL1, and EL0 can access the coprocessor,
    // floating point, and SIMD instructions and registers
    ldr     \xreg1, =CPTR_EL2_INIT_VALUE
    msr     cptr_el2, \xreg1
    ldr     \xreg1, =HSTR_EL2_INIT_VALUE
    msr     hstr_el2, \xreg1
    ldr     \xreg1, =CPACR_EL1_INIT_VALUE
    msr     cpacr_el1, \xreg1

    // Overwrite the exception return address with the location of the first 1
    // label past this instruction, and then return to it (in EL1)
    adr     \xreg1, 1f
    msr     elr_el2, \xreg1
    eret
1:
    .endm

// Where the GPU will start execution (put at the correct address via our link.ld file)
.section ".text.boot"

.globl _start

// RPi3 has four processors, all of which start executing this code. The
// mpidr_el1 register contains a variety of processor information, but we're
// only interested in the ID, which exists in the low byte of the register
// value. So mask out the low byte to get the processor ID and if it's 0, boot
// up. Otherwise hang the core by putting it into an infinite loop.
// TODO: Latest firmware may have changed this so only CPU 0 starts up, with
// the other three sleeping, requiring a memory write to wake each.
_start:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbz     x0, master
    b       proc_hang

// Infinite loop to hang the other processors with
proc_hang:
    b       proc_hang

// Main processor starts here
master:
    // See if our exception level is 1 already, if it is, skip elevation.
    // Exception level is in bits 2 and 3
    mrs     x0, CurrentEL
    cmp     x0, #(1 << 2)
    beq     1f

    armv8_switch_to_el1 x0

1:
    // Load the start of the .bss segment into x0 and the count into x1 (32
    // bits) then loop and store zero into every 64-bit chunk
    adr     x0, __bss_start
    ldr     w1, =__bss_size
    cbz     w1, start_kmain
clear_bss:
    str     xzr, [x0], #8       // Stores 0 into *x0 and adds 8
    sub     w1, w1, #1          // Decrement w1
    cbnz    w1, clear_bss

start_kmain:
    bl      __create_page_tables

    // Update the stack pointer to point at the virtual address instead of a physical one
    mov     x0, #VA_START
    add     sp, x0, #LOW_MEMORY

    // Update the table translation register to point at our page directory
    adrp    x0, pg_dir
    msr     ttbr1_el1, x0

    // Set up the translation control register for stage 1 of the EL1/0 translation regime
    ldr     x0, =(TCR_VALUE)
    msr     tcr_el1, x0

    // Set up the MAIR register with the flags we use for memory types
    ldr     x0, =(MAIR_VALUE)
    msr     mair_el1, x0

    // Load the absolute address of our kernel main function into x2 BEFORE we turn on the MMU
    ldr     x2, =kmain

    // Now turn on the MMU
    mov     x0, #SCTLR_MMU_ENABLED
    msr     sctlr_el1, x0

    // And jump to the kmain address we stored previously
    br      x2
    b       proc_hang           // hang the processor if kmain ever exits

    /////////////////////////////////////////////
    // Creates the Page Global Directory and the Page Upper Directory
    // table_start - pointer to the memory where the tables are to be allocated (updated to point after the PUD)
    // target_virtual_addr - virtual address currently being mapped
    // temp1, temp2 - temporary registers for use
    /////////////////////////////////////////////
    .macro  create_pgd_entry, table_start, target_virtual_addr, temp1, temp2
    create_table_entry \table_start, \target_virtual_addr, PGD_SHIFT, \temp1, \temp2
    create_table_entry \table_start, \target_virtual_addr, PUD_SHIFT, \temp1, \temp2
    /////////////////////////////////////////////
    .endm
    /////////////////////////////////////////////

    /////////////////////////////////////////////
    // "Allocates" a page table. (The Page Global Directory and Page Upper Directory)
    // table_start - pointer to the memory where the table is to be allocated (updated to point at next table)
    // target_virtual_addr - virtual address currently being mapped
    // index_shift - shift that needs to be applied to the virtual address in order to get the current table index
    // temp1, temp2 - temporary registers for use
    /////////////////////////////////////////////
    .macro  create_table_entry, table_start, target_virtual_addr, index_shift, temp1, temp2

    // shift the address to strip off anything to the right of the table index, then AND it with the maximum index
    // to strip off anything to the left, leaving temp1 holding the index this table is for
    lsr     \temp1, \target_virtual_addr, #\index_shift
    and     \temp1, \temp1, #PTRS_PER_TABLE - 1

    // calculate the starting address of the next table (stored contiguously after this one)
    add     \temp2, \table_start, #PAGE_SIZE

    // flag the descriptor as pointing at a table, and being valid
    orr     \temp2, \temp2, #MM_TYPE_PAGE_TABLE

    // store the descriptor (in temp2) into the table at the index temp1 is pointing at
    str     \temp2, [\table_start, \temp1, lsl #3]

    // update the table start to point at the next table
    add     \table_start, \table_start, #PAGE_SIZE
    /////////////////////////////////////////////
    .endm
    /////////////////////////////////////////////

    /////////////////////////////////////////////
    // Creates the Page Middle Directory
    // table_start - pointer to the PMD table being filled
    // target_physical_region - start of the physical region being mapped
    // virtual_start - the virtual address of the first section being mapped
    // virtual_end - the virtual address of the last section being mapped
    // flags - the flags to be inserted into the block descriptor
    // temp1 - temporary register for use
    /////////////////////////////////////////////
    .macro  create_block_map, table_start, target_physical_region, virtual_start, virtual_end, flags, temp1

    // shift the address and then AND it with the number of entries to extract the table index
    lsr     \virtual_start, \virtual_start, #SECTION_SHIFT
    and     \virtual_start, \virtual_start, #PTRS_PER_TABLE - 1
    // repeat with the end pointer to get the ending index
    lsr     \virtual_end, \virtual_end, #SECTION_SHIFT
    and     \virtual_end, \virtual_end, #PTRS_PER_TABLE - 1

    // shift the address right to trim off the bottom bits, then shift it back left and apply the flags in the now
    // empty bits to form the block descriptor
    lsr     \target_physical_region, \target_physical_region, #SECTION_SHIFT
    mov     \temp1, #\flags
    orr     \target_physical_region, \temp1, \target_physical_region, lsl #SECTION_SHIFT

    // loop to store the descriptor in target_physical_region into the table, then increment the current index in
    // virtual_start, and the descriptor to point at the next section, and loop until start equals the end
9999:
    str     \target_physical_region, [\table_start, \virtual_start, lsl #3]
    add     \virtual_start, \virtual_start, #1
    add     \target_physical_region, \target_physical_region, #SECTION_SIZE
    cmp     \virtual_start, \virtual_end
    b.ls    9999b
    /////////////////////////////////////////////
    .endm
    /////////////////////////////////////////////

__create_page_tables:
    // Save the return address into x29 (we don't need to use a stack since we're not recursing, and x29 shouldn't
    // be clobbered)
    mov     x29, x30

    // Zero out the page directory section of the kernel image
    adrp    x0, pg_dir
    mov     x1, #PG_DIR_SIZE
clear_pg_dir:
    str     xzr, [x0], #8
    sub     x1, x1, #1
    cbnz    x1, clear_pg_dir

    // Create our PGD and PUD in the directory section of the kernel image, mapping our kernel virtual addresses
    // x2 and x3 are used as scratch registers
    adrp    x0, pg_dir      // table_start
    mov     x1, #VA_START   // target_virtual_addr
    create_pgd_entry x0, x1, x2, x3
    // x0 now contains the start of the next table (the PMD)

    // Map all of memory, excluding the device area, with x4 as a scratch register. Note that table_start is already
    // in x0 from previous macro
    mov     x1, xzr                                         // target_physical_region
    mov     x2, #VA_START                                   // virtual_start
    ldr     x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)    // virtual_end
    create_block_map x0, x1, x2, x3, MMU_FLAGS, x4
    // again, x0 now contains the start of the next table

    // Map the rest of memory as device memory, with x4 as a scratch register. Again, table_start is already in x0
    mov     x1, #DEVICE_BASE                                    // target_physical_region
    ldr     x2, =(VA_START + DEVICE_BASE)                       // virtual_start
    ldr     x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)   // virtual_end
    create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

    // Restore the return address from where we saved it
    mov     x30, x29
    ret
