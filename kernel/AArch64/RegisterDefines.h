// Included from assembly, so can't have anything fancy in here

#ifndef KERNEL_AARCH64_REGISTER_DEFINES_H
#define KERNEL_AARCH64_REGISTER_DEFINES_H

///////////////////////////////////////////////////////////////////////////////
// ESR_ELx register flags - See D10.2.39 in the ARMv8 manual
///////////////////////////////////////////////////////////////////////////////

// How far to shift the value in an ESR_ELx register to get at the exception class
#define ESR_ELx_EC_SHIFT    26

//Exception classes:
#define ESR_ELx_EC_SVC64    0x15    // 0b010101 - exception caused by SVC instruction in AArch64 state
#define ESR_ELx_EC_DABT_LOW 0x24    // 0b100100 - data abort from a lower exception level

#endif // KERNEL_AARCH64_REGISTER_DEFINES_H