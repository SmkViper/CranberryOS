#include "SystemRegisters.h"

namespace AArch64
{
    
    void CPACR_EL1::Write(CPACR_EL1 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr cpacr_el1, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    CPACR_EL1 CPACR_EL1::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], cpacr_el1"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return CPACR_EL1{ readRawValue };
    }

    void CPACR_EL1::FPEN(FPENTraps const aTraps)
    {
        RegisterValue &= std::bitset<64>{ ~(FPENIndex_Mask) };
        auto const maskedTraps = (static_cast<uint8_t>(aTraps) << FPENIndex_Shift) & FPENIndex_Mask;
        RegisterValue |= std::bitset<64>{ maskedTraps };
    }

    CPACR_EL1::FPENTraps CPACR_EL1::FPEN() const
    {
        return static_cast<FPENTraps>((RegisterValue & std::bitset<64>{ FPENIndex_Mask }).to_ulong());
    }

    void CPTR_EL2::Write(CPTR_EL2 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr cptr_el2, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    CPTR_EL2 CPTR_EL2::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], cptr_el2"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return CPTR_EL2{ readRawValue };
    }

    void HCR_EL2::Write(HCR_EL2 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr hcr_el2, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    HCR_EL2 HCR_EL2::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], hcr_el2"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return HCR_EL2{ readRawValue };
    }

    void HSTR_EL2::Write(HSTR_EL2 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr hstr_el2, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    HSTR_EL2 HSTR_EL2::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], hstr_el2"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return HSTR_EL2{ readRawValue };
    }

    void SPSR_EL2::Write(SPSR_EL2 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr spsr_el2, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    SPSR_EL2 SPSR_EL2::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], spsr_el2"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return SPSR_EL2{ readRawValue };
    }

    void SPSR_EL2::M(Mode const aMode)
    {
        RegisterValue &= std::bitset<64>{ ~(MIndex_Mask) };
        auto const maskedMode = static_cast<uint8_t>(aMode) & MIndex_Mask;
        RegisterValue |= std::bitset<64>{ maskedMode };
    }

    SPSR_EL2::Mode SPSR_EL2::M() const
    {
        return static_cast<Mode>((RegisterValue & std::bitset<64>{ MIndex_Mask }).to_ulong());
    }
}