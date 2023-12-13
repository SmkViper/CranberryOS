#include "SystemRegisters.h"

namespace AArch64
{
    namespace
    {
        /**
         * Writes a multi-bit value to a bitset
         * 
         * @param arBitset The bitset to modify
         * @param aValue The value to write
         * @param aMask An un-shifted mask for the bits to write
         * @param aShit How much to shift the value before writing
        */
        template<typename EnumType, size_t BitsetSize>
        void WriteMultiBitValue(std::bitset<BitsetSize>& arBitset, EnumType const aValue, uint64_t const aMask, uint64_t const aShift)
        {
            arBitset &= std::bitset<64>{ ~(aMask << aShift) };
            auto const maskedValue = (static_cast<uint64_t>(aValue) & aMask) << aShift;
            arBitset |= std::bitset<64>{ maskedValue };
        }

        /**
         * Reads a multi-bit value from a bitset
         * 
         * @param aBitset The bitset to read
         * @param aMask An un-shifted mask fro the bits to read
         * @param aShift How much to shift the value after reading
         * @return The read bits, casted to EnumType
        */
        template<typename EnumType, size_t BitsetSize>
        EnumType ReadMultiBitValue(std::bitset<BitsetSize> const& aBitset, uint64_t const aMask, uint64_t const aShift)
        {
            auto const shiftedMask = aMask << aShift;
            return static_cast<EnumType>((aBitset & std::bitset<64>{ shiftedMask }).to_ulong() >> aShift);
        }
    }
    
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
        WriteMultiBitValue(RegisterValue, aTraps, FPENIndex_Mask, FPENIndex_Shift);
    }

    CPACR_EL1::FPENTraps CPACR_EL1::FPEN() const
    {
        return ReadMultiBitValue<FPENTraps>(RegisterValue, FPENIndex_Mask, FPENIndex_Shift);
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
        WriteMultiBitValue(RegisterValue, aMode, MIndex_Mask, MIndex_Shift);
    }

    SPSR_EL2::Mode SPSR_EL2::M() const
    {
        return ReadMultiBitValue<Mode>(RegisterValue, MIndex_Mask, MIndex_Shift);
    }
}