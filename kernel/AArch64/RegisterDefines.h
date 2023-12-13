// Included from assembly, so can't have anything fancy in here

#ifndef KERNEL_AARCH64_REGISTER_DEFINES_H
#define KERNEL_AARCH64_REGISTER_DEFINES_H

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
#define ESR_ELx_EC_DABT_LOW 0x24    // 0b100100 - data abort from a lower exception level

#endif // KERNEL_AARCH64_REGISTER_DEFINES_H