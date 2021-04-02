// Included from assembly, so can't have anything fancy in here

#ifndef KERNEL_ARM_SYSTEM_REGISTERS_H
#define KERNEL_ARM_SYSTEM_REGISTERS_H

///////////////////////////////////////////////////////////////////////////////
// SCTLR_EL1 register flags - See D10.2.100 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// #TODO: We need to get these out of defines if possible

// Some bits are reserved, set to 1, in ARMv8.0, which we set here
#define SCTLR_RESERVED_FLAGS    (1 << 29) | (1 << 28) | (1 << 23) | (1 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_EL1_LITTLE_ENDIAN (0 << 25) // EL1 data access and exceptions are little-endian
#define SCTLR_EL0_LITTLE_ENDIAN (0 << 24) // EL0 data access is little-endian
#define SCTLR_ICACHE_DISABLED   (0 << 12) // Disable the instruction cache
#define SCTLR_DCACHE_DISABLED   (0 << 2) // Disable the data cache
#define SCTLR_MMU_DISABLED      (0 << 0) // Disable memory address translation
#define SCTLR_MMU_ENABLED       (1 << 0) // Enable memory address translation

// Simplest mode possible for now - pretty much set the CPU in little-endian mode with caching and address
// translation disabled (we don't have virtual memory set up yet)
#define SCTLR_INIT_VALUE    (SCTLR_RESERVED_FLAGS | \
                             SCTLR_EL1_LITTLE_ENDIAN | \
                             SCTLR_EL0_LITTLE_ENDIAN | \
                             SCTLR_ICACHE_DISABLED | \
                             SCTLR_DCACHE_DISABLED | \
                             SCTLR_MMU_DISABLED)

///////////////////////////////////////////////////////////////////////////////
// HCR_EL2 register flags - See D10.2.45 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// Some bits are reserved, but none are set to 1, so this one is simple
#define HCR_EL2_RESERVED_FLAGS  0
#define HCR_EL2_EL1_IS_AARCH64  (1 << 31) // set EL1 to AArch64 mode

// We aren't going to implement a hypervisor for now, so only thing we need to set is to make sure that
// EL1 executes in AArch64 mode instead of AArch32
#define HCR_EL2_INIT_VALUE  (HCR_EL2_RESERVED_FLAGS | \
                             HCR_EL2_EL1_IS_AARCH64)

///////////////////////////////////////////////////////////////////////////////
// SPSR_EL2 register flags - See C5.2.19 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// Some bits are reserved, but none are set to 0, so this one is simple
#define SPSR_EL2_RESERVED_FLAGS                 0
#define SPSR_EL2_PROCESS_STATE_INTERRUPT_MASK   (1 << 9)
#define SPSR_EL2_SERROR_INTERRUPT_MASK          (1 << 8)
#define SPSR_EL2_IRQ_MASK                       (1 << 7)
#define SPSR_EL2_FIQ_MASK                       (1 << 6)
#define SPSR_EL2_EL1_USES_OWN_SP                (1 << 2) | 1 // EL1 uses its own stack pointer instead of SP0

// Mask out (disable) all interrupts and tell EL1 to use its own stack pointer
#define SPSR_EL2_INIT_VALUE (SPSR_EL2_RESERVED_FLAGS | \
                             SPSR_EL2_PROCESS_STATE_INTERRUPT_MASK | \
                             SPSR_EL2_SERROR_INTERRUPT_MASK | \
                             SPSR_EL2_IRQ_MASK | \
                             SPSR_EL2_FIQ_MASK | \
                             SPSR_EL2_EL1_USES_OWN_SP)

///////////////////////////////////////////////////////////////////////////////
// CPTR_EL2 register flags - See D10.2.30 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// Some bits are reserved, set to 1, which we set here
#define CPTR_EL2_RESERVED_FLAGS             (1 << 13) | (1 << 12) | (1 << 9) | (1 << 8) | 0xFF
#define CPTR_EL2_DISABLE_SEV_FP_SIMD_TRAPS  (0 << 10)

// AArch64: Allow EL2 and non-secure EL1 and EL0 to access SEV, floating-point, and SIMD registers (won't trap to EL2)
#define CPTR_EL2_INIT_VALUE (CPTR_EL2_RESERVED_FLAGS | \
                             CPTR_EL2_DISABLE_SEV_FP_SIMD_TRAPS)

///////////////////////////////////////////////////////////////////////////////
// HSTR_EL2 register flags - See D10.2.47 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// Some bits are reserved, but none are set to 0, so this one is simple
#define HSTR_EL2_RESERVED_FLAGS 0
// Access to coprocessor registers are set by bits [15:0], set to 1 to trap, 0 to not trap. So just leave everything
// zeroed to not trap

// AArch32: Allow non-secure EL1 and EL0 access all coprocessor registers (won't trap to EL2)
#define HSTR_EL2_INIT_VALUE     HSTR_EL2_RESERVED_FLAGS

///////////////////////////////////////////////////////////////////////////////
// CPACR_EL1 register flags - See D10.2.29 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// Some bits are reserved, but none are set to 0, so this one is simple
#define CPACR_EL1_RESERVED_FLAGS            0
#define CPACR_EL1_DISABLE_SVE_FP_SIMD_TRAPS (3 << 20)

// AArch64: Allow EL1 and EL0 to use SVE, floating-point, and SIMD registers (won't trap to EL1)
#define CPACR_EL1_INIT_VALUE    (CPACR_EL1_RESERVED_FLAGS | \
                                 CPACR_EL1_DISABLE_SVE_FP_SIMD_TRAPS)

///////////////////////////////////////////////////////////////////////////////
// ESR_ELx register flags - See D10.2.39 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// How far to shift the value in an ESR_ELx register to get at the exception class
#define ESR_ELx_EC_SHIFT    26

//Exception classes:
#define ESR_ELx_EC_SVC64    0x15    // 0b010101 - exception caused by SVC instruction in AArch64 state

#endif // KERNEL_ARM_SYSTEM_REGISTERS_H