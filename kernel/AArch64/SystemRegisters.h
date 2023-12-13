#ifndef KERNEL_AARCH64_SYSTEM_REGISTERS_H
#define KERNEL_AARCH64_SYSTEM_REGISTERS_H

#include <bitset>
#include <cstdint>

namespace AArch64
{
    /**
     * Architectural Feature Access Control Register
     * https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/CPACR-EL1--Architectural-Feature-Access-Control-Register
    */
    class CPACR_EL1
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed
        */
        CPACR_EL1() = default;

        /**
         * Writes the given value to the CPACR_EL1 register
         * 
         * @param aValue Value to write
        */
        static void Write(CPACR_EL1 const aValue);

        /**
         * Reads the current state of the CPACR_EL1 register
         * 
         * @return The current state of the register
        */
        static CPACR_EL1 Read();

        enum class FPENTraps: uint8_t
        {
            TrapAll = 0b00,
            TrapEL0 = 0b01,
            TrapAll2 = 0b10, // documented as the same as TrapAll, not sure the difference?
            TrapNone = 0b11,
        };

        /**
         * FPEN bits - controls traps of floating point and SIMD instructions
         * 
         * @param aTraps The traps for FPEN
        */
        void FPEN(FPENTraps const aTraps);

        /**
         * FPEN bits - controls traps of floating point and SIMD instructions
         * 
         * @return The current traps
        */
        FPENTraps FPEN() const;

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        CPACR_EL1(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // Reserved     [15:0]
        // #TODO: ZEN   [17:16] (Res0 if FEAT_SVE is not available)
        // Reserved     [19:18]
        static constexpr uint64_t FPENIndex_Shift = 20;
        static constexpr uint64_t FPENIndex_Mask = 0b11 << FPENIndex_Shift;
        // Reserved     [27:22]
        // #TODO: TTA   [28]
        // Reserved     [63:29]

        std::bitset<64> RegisterValue;
    };

    /**
     * Architectural Feature Trap Register (EL2)
     * https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/CPTR-EL2--Architectural-Feature-Trap-Register--EL2-
     * 
     * Note that the definition of this class assumes HCR_EL2.E2H is 0. But since we never expect to turn it on (it's
     * apparently a feature that lets the OS run in EL2 with the programs in EL0) this should be fine.
    */
    class CPTR_EL2
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed (and Res1 bits set)
        */
        CPTR_EL2()
            : CPTR_EL2{ ReservedValues }
        {}

        /**
         * Writes the given value to the CPTR_EL2 register
         * 
         * @param aValue Value to write
        */
        static void Write(CPTR_EL2 const aValue);

        /**
         * Reads the current state of the CPTR_EL2 register
         * 
         * @return The current state of the register
        */
        static CPTR_EL2 Read();

        /**
         * TFP Bit - Traps execution of instructions for SIMD and floating-point functionality
         * 
         * @param aTrapFPInstructions If true, SIMD and floating point instructions will trap to EL2
        */
        void TFP(bool const aTrapFPInstructions) { RegisterValue[TFPIndex] = aTrapFPInstructions; }

        /**
         * TFP Bit - Traps execution of instructions for SIMD and floating-point functionality
         * 
         * @return True if SIMD and floating point instructions trap to EL2
        */
        bool TFP() const { return RegisterValue[TFPIndex]; }

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        CPTR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // We are currently assuming FEAT_SVE isn't available, so bit 8 is Res1
        static constexpr uint64_t ReservedValues = (1 << 13) | (1 << 12) | (1 << 9) | (1 << 8) | 0xFF;

        // Reserved     [7:0] (Res1)
        // TZ           [8] (Res1 if FEAT_SVE is not available)
        // Reserved     [9] (Res1)
        static constexpr unsigned TFPIndex = 10;
        // Reserved     [11]
        // Reserved     [13:12] (Res1)
        // Reserved     [19:14]
        // #TODO: TTA   [20]
        // Reserved     [29:21]
        // #TODO: TAM   [30]
        // #TODO: TCPAC [31]
        // Reserved     [63:32]

        std::bitset<64> RegisterValue;
    };

    /**
     * Hypervisor Configuration Register
     * https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register
    */
    class HCR_EL2
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed
        */
        HCR_EL2() = default;

        /**
         * Writes the given value to the HCR_EL2 register
         * 
         * @param aValue Value to write
        */
        static void Write(HCR_EL2 const aValue);

        /**
         * Reads the current state of the HCR_EL2 register
         * 
         * @return The current state of the register
        */
        static HCR_EL2 Read();

        /**
         * RW Bit - Execution state for lower exception levels
         * 
         * @param aEL1ExecutionIsAArch64 If true, EL1 will be AArch64, otherwise it will be AArch32
        */
        void RW(bool const aEL1ExecutionIsAArch64) { RegisterValue[RWIndex] = aEL1ExecutionIsAArch64; }

        /**
         * RW Bit - Execution state for lower exception levels
         * 
         * @return True if EL1 execution state is AArch64. Otherwise it's AArch32
        */
        bool RW() const { return RegisterValue[RWIndex]; }

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        HCR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // #TODO: VM    [0]
        // #TODO: SWIO  [1]
        // #TODO: PTW   [2]
        // #TODO: FMO   [3]
        // #TODO: IMO   [4]
        // #TODO: AMO   [5]
        // #TODO: VF    [6]
        // #TODO: VI    [7]
        // #TODO: VSE   [8]
        // #TODO: FB    [9]
        // #TODO: BSU   [11:10]
        // #TODO: DC    [12]
        // #TODO: TWI   [13]
        // #TODO: TWE   [14]
        // #TODO: TID0  [15]
        // #TODO: TID1  [16]
        // #TODO: TID2  [17]
        // #TODO: TID3  [18]
        // #TODO: TSC   [19]
        // #TODO: TIDCP [20]
        // #TODO: TACR  [21]
        // #TODO: TSW   [22]
        // #TODO: TPCP  [23]
        // #TODO: TPU   [24]
        // #TODO: TTLB  [25]
        // #TODO: TVM   [26]
        // #TODO: TGE   [27]
        // #TODO: TDZ   [28]
        // #TODO: HCD   [29]
        // #TODO: TRVM  [30]
        static constexpr unsigned RWIndex = 31;
        // #TODO: CD    [32]
        // #TODO: ID    [33]
        // #TODO: E2H   [34]
        // #TODO: TLOR  [35]
        // #TODO: TERR  [36]
        // #TODO: TEA   [37]
        // #TODO: MIOCNCE [38]
        // Reserved     [39]
        // #TODO: APK   [40]
        // #TODO: API   [41]
        // #TODO: NV    [42]
        // #TODO: NV1   [43]
        // #TODO: AT    [44]
        // #TODO: NV2   [45]
        // #TODO: FWB   [46]
        // #TODO: FIEN  [47]
        // Reserved     [48]
        // #TODO: TID4  [49]
        // #TODO: TICAB [50]
        // #TODO: AMVOFFEN [51]
        // #TODO: TOCU  [52]
        // #TODO: EnSCXT [53]
        // #TODO: TTLBIS [54]
        // #TODO: TTLBOS [55]
        // #TODO: ATA   [56]
        // #TODO: DCT   [57]
        // #TODO: TID5  [58]
        // #TODO: TWEDEn [59]
        // #TODO: TWEDEL [63:60]

        std::bitset<64> RegisterValue;
    };

    /**
     * Hypervisor System Trap Register
     * https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/HSTR-EL2--Hypervisor-System-Trap-Register
    */
    class HSTR_EL2
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed
        */
        HSTR_EL2() = default;

        /**
         * Writes the given value to the HSTR_EL2 register
         * 
         * @param aValue Value to write
        */
        static void Write(HSTR_EL2 const aValue);

        /**
         * Reads the current state of the HSTR_EL2 register
         * 
         * @return The current state of the register
        */
        static HSTR_EL2 Read();

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        HSTR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // #TODO: T0    [0]
        // #TODO: T1    [1]
        // #TODO: T2    [2]
        // #TODO: T3    [3]
        // Reserved     [4]
        // #TODO: T5    [5]
        // #TODO: T6    [6]
        // #TODO: T7    [7]
        // #TODO: T8    [8]
        // #TODO: T9    [9]
        // #TODO: T10   [10]
        // #TODO: T11   [11]
        // #TODO: T12   [12]
        // #TODO: T13   [13]
        // Reserved     [14]
        // #TODO: T15   [15]
        // Reserved     [63:16]

        std::bitset<64> RegisterValue;
    };

    /**
     * Saved Program Status Register (EL2)
     * https://developer.arm.com/documentation/ddi0601/2023-09/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-
    */
    class SPSR_EL2
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed
        */
        SPSR_EL2() = default;

        /**
         * Writes the given value to the SPSR_EL2 register
         * 
         * @param aValue Value to write
        */
        static void Write(SPSR_EL2 const aValue);

        /**
         * Reads the current state of the SPSR_EL2 register
         * 
         * @return The current state of the register
        */
        static SPSR_EL2 Read();

        enum class Mode: uint8_t
        {
            EL0t = 0b0000, // Exception level 0
            EL1t = 0b0100, // Exception level 1, SP is SP_EL0 (shared stack)
            EL1h = 0b0101, // Exception level 1, SP is SP_EL1 (own stack)
            EL2t = 0b1000, // Exception level 2, SP is SP_EL0 (shared stack)
            EL2h = 0b1001, // Exception level 2, SP is SP_EL2 (own stack)
        };

        /**
         * Mode bits - where to return to with ERET and whether to use its own stack or not
         * 
         * @param aMode The mode for ERET
        */
        void M(Mode const aMode);

        /**
         * Mode bits - where to return to with ERET and whether to use its own stack or not
         * 
         * @return The current ERET mode
        */
        Mode M() const;

        /**
         * F Bit - FIQ interrupt mask
         * 
         * @param aFIQInterruptMask Masks (ignores) FIQ interrupts if set
        */
        void F(bool const aFIQInterruptMask) { RegisterValue[FIndex] = aFIQInterruptMask; }

        /**
         * F Bit - FIQ interrupt mask
         * 
         * @return True if FIQ interrupts are masked (ignored)
        */
        bool F() const { return RegisterValue[FIndex]; }

        /**
         * I Bit - IRQ interrupt mask
         * 
         * @param aIRQInterruptMask Masks (ignores) IRQ interrupts if set
        */
        void I(bool const aIRQInterruptMask) { RegisterValue[IIndex] = aIRQInterruptMask; }

        /**
         * I Bit - IRQ interrupt mask
         * 
         * @return True if IRQ interrupts are masked (ignored)
        */
        bool I() const { return RegisterValue[IIndex]; }

        /**
         * A Bit - SError interrupt mask
         * 
         * @param aSErrorInterruptMask Masks (ignores) SError interrupts if set
        */
        void A(bool const aSErrorInterruptMask) { RegisterValue[AIndex] = aSErrorInterruptMask; }

        /**
         * A Bit - SError interrupt mask
         * 
         * @return True if SError interrupts are masked (ignored)
        */
        bool A() const { return RegisterValue[AIndex]; }

        /**
         * D Bit - Debug exception mask
         * 
         * @param aDebugExceptionMask Masks (ignores) debug exceptions if set
        */
        void D(bool const aDebugExceptionMask) { RegisterValue[DIndex] = aDebugExceptionMask; }

        /**
         * D Bit - Debug exception mask
         * 
         * @return True if debug exceptions are masked (ignored)
        */
        bool D() const { return RegisterValue[DIndex]; }

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        SPSR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        static constexpr uint64_t MIndex_Mask = 0b1111;
        // #TODO: M[4]  [4]
        // Reserved     [5]
        static constexpr unsigned FIndex = 6;
        static constexpr unsigned IIndex = 7;
        static constexpr unsigned AIndex = 8;
        static constexpr unsigned DIndex = 9;
        // #TODO: BTYPE [11:10]
        // #TODO: SSBS  [12]
        // Reserved     [19:13]
        // #TODO: IL    [20]
        // #TODO: SS    [21]
        // #TODO: PAN   [22]
        // #TODO: UAO   [23]
        // #TODO: DIT   [24]
        // #TODO: TCO   [25]
        // Reserved     [27:26]
        // #TODO: V     [28]
        // #TODO: C     [29]
        // #TODO: Z     [30]
        // #TODO: N     [31]
        // Reserved:    [63:32]

        std::bitset<64> RegisterValue;
    };
}

#endif // KERNEL_AARCH64_SYSTEM_REGISTERS_H