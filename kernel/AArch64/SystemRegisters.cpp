#include "SystemRegisters.h"

#include <cstring>

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

    MAIR_EL1::Attribute MAIR_EL1::Attribute::NormalMemory()
    {
        // #TODO: Figure out if this needs to change and what these words mean

        // Normal memory, outer non-cacheable
        // Normal memory, inner non-cacheable
        return Attribute{ 0b0100'0100 };
    }

    MAIR_EL1::Attribute MAIR_EL1::Attribute::DeviceMemory()
    {
        // #TODO: Figure out if this needs to change and what these words mean

        // Device nGnRnE memory
        // Non-gathering (one access in code = one access on bus)
        // Non-reordering (disallows reordering of access)
        // Non-early write acknowledgement (responses come from end slave, not buffering in the interconnect)
        return Attribute{ 0b0000'0000 };
    }

    /**
     * Construct the register data from a raw 64-bit value from the register
     * 
     * @param aRawValue The raw register value to insert
    */
    MAIR_EL1::MAIR_EL1(uint64_t const aRawValue)
    {
        static_assert(sizeof(uint64_t) == sizeof(MAIR_EL1::Attributes), "Unexpected size mismatch");
        std::memcpy(Attributes, &aRawValue, sizeof(Attributes));
    }

    void MAIR_EL1::Write(MAIR_EL1 const aValue)
    {
        static_assert(sizeof(uint64_t) == sizeof(MAIR_EL1::Attributes), "Unexpected size mismatch");
        uint64_t rawValue = 0;
        std::memcpy(&rawValue, aValue.Attributes, sizeof(rawValue));
        
        asm volatile(
            "msr mair_el1, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    MAIR_EL1 MAIR_EL1::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], mair_el1"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return MAIR_EL1{ readRawValue };
    }

    void MAIR_EL1::SetAttribute(size_t const aIndex, Attribute const aValue)
    {
        // #TODO: Panic if index is out of range
        Attributes[aIndex] = aValue.Value;
    }

    MAIR_EL1::Attribute MAIR_EL1::GetAttribute(size_t const aIndex) const
    {
        // #TODO: Panic if index is out of range
        return Attribute{ Attributes[aIndex] };
    }

    void SCTLR_EL1::Write(SCTLR_EL1 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr sctlr_el1, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    SCTLR_EL1 SCTLR_EL1::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], sctlr_el1"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return SCTLR_EL1{ readRawValue };
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

    void TCR_EL1::Write(TCR_EL1 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr tcr_el1, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    TCR_EL1 TCR_EL1::Read()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], tcr_el1"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return TCR_EL1{ readRawValue };
    }

    void TCR_EL1::T0SZ(uint8_t const aBits)
    {
        // #TODO: Panic if bits are out of range - might depend on the hardware, but anything over 52 is too much

        // The address size in bytes is 2^(64 - TnSZ)
        //
        // So in order to convert the number of bits to the TnSZ value representing that number of bits, we need to
        // use (64 - aBits).
        //
        // For example, if you want the address size to be 48 bits (0x0000'0000'0000'0000 - 0x0000'FFFF'FFFF'FFFF),
        // then (64 - TnSZ) must be 48, which means TnSZ should be 16, which is (64 - 48).
        auto const encodedValue = (64 - aBits);
        WriteMultiBitValue(RegisterValue, encodedValue, T0SZIndex_Mask, T0SZIndex_Shift);
    }

    uint8_t TCR_EL1::T0SZ() const
    {
        // The address size in bytes is 2^(64 - TnSZ)
        //
        // So in order to convert the TnSZ value to the number of bits in the address range, we just use 
        // (64 - encodedValue)
        auto const encodedValue = ReadMultiBitValue<uint8_t>(RegisterValue, T0SZIndex_Mask, T0SZIndex_Shift);
        return 64 - encodedValue;
    }

    void TCR_EL1::TG0(T0Granule const aSize)
    {
        WriteMultiBitValue(RegisterValue, aSize, TG0Index_Mask, TG0Index_Shift);
    }

    TCR_EL1::T0Granule TCR_EL1::TG0() const
    {
        return ReadMultiBitValue<T0Granule>(RegisterValue, TG0Index_Mask, TG0Index_Shift);
    }

    void TCR_EL1::T1SZ(uint8_t const aBits)
    {
        // #TODO: Panic if bits are out of range - might depend on the hardware, but anything over 52 is too much

        // See T0SZ for explanation of encoding
        auto const encodedValue = (64 - aBits);
        WriteMultiBitValue(RegisterValue, encodedValue, T1SZIndex_Mask, T1SZIndex_Shift);
    }

    uint8_t TCR_EL1::T1SZ() const
    {
        // See T0SZ for explanation of encoding
        auto const encodedValue = ReadMultiBitValue<uint8_t>(RegisterValue, T1SZIndex_Mask, T1SZIndex_Shift);
        return 64 - encodedValue;
    }

    void TCR_EL1::TG1(T1Granule const aSize)
    {
        WriteMultiBitValue(RegisterValue, aSize, TG1Index_Mask, TG1Index_Shift);
    }

    TCR_EL1::T1Granule TCR_EL1::TG1() const
    {
        return ReadMultiBitValue<T1Granule>(RegisterValue, TG1Index_Mask, TG1Index_Shift);
    }

    void TTBRn_EL1::Write0(TTBRn_EL1 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr ttbr0_el1, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    void TTBRn_EL1::Write1(TTBRn_EL1 const aValue)
    {
        uint64_t const rawValue = aValue.RegisterValue.to_ulong();
        asm volatile(
            "msr ttbr1_el1, %[value]"
            : // no outputs
            :[value] "r"(rawValue) // inputs
            : // no bashed registers
        );
    }

    TTBRn_EL1 TTBRn_EL1::Read0()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], ttbr0_el1"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return TTBRn_EL1{ readRawValue };
    }

    TTBRn_EL1 TTBRn_EL1::Read1()
    {
        uint64_t readRawValue = 0;
        asm volatile(
            "mrs %[value], ttbr1_el1"
            :[value] "=r"(readRawValue) // outputs
            : // no inputs
            : // no bashed registers
        );
        return TTBRn_EL1{ readRawValue };
    }

    void TTBRn_EL1::BADDR(uintptr_t const aBaseAddress)
    {
        WriteMultiBitValue(RegisterValue, aBaseAddress, BADDRIndex_Mask, BADDRIndex_Shift);
    }

    uintptr_t TTBRn_EL1::BADDR() const
    {
        return ReadMultiBitValue<uint64_t>(RegisterValue, BADDRIndex_Mask, BADDRIndex_Shift);
    }
}