#ifndef KERNEL_AARCH64_SYSTEM_REGISTERS_H
#define KERNEL_AARCH64_SYSTEM_REGISTERS_H

#include <bitset>
#include <cstdint>

// Terminology:
// https://developer.arm.com/documentation/105565/latest/
// Res0: Write 0 to initialize, then preserve value (read-modify-write)
// Res1: Write 1 to initialize, then preserve value (read-modify-write)
// RAZ/WI: Hardwired to read as 0 and ignore writes
//
// For FEAT_ names:
// https://developer.arm.com/downloads/-/exploration-tools/feature-names-for-a-profile

namespace UnitTests::AArch64::SystemRegisters::Details
{
    struct TestAccessor;
}

namespace AArch64
{
    /**
     * Architectural Feature Access Control Register
     * https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/CPACR-EL1--Architectural-Feature-Access-Control-Register
    */
    class CPACR_EL1
    {
        friend struct UnitTests::AArch64::SystemRegisters::Details::TestAccessor;
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
        static void Write(CPACR_EL1 aValue);

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
        void FPEN(FPENTraps aTraps);

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
        explicit CPACR_EL1(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // Reserved     [15:0]  (Res0)
        // ZEN          [17:16] (Res0 if FEAT_SVE is not available)
        // Reserved     [19:18] (Res0)
        static constexpr unsigned FPENIndex_Shift = 20; // bits [20:21]
        static constexpr uint64_t FPENIndex_Mask = 0b11;
        // Reserved     [27:22] (Res0)
        // TTA          [28]
        // Reserved     [63:29] (Res0)

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
        friend struct UnitTests::AArch64::SystemRegisters::Details::TestAccessor;
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
        static void Write(CPTR_EL2 aValue);

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
        explicit CPTR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // We are currently assuming FEAT_SVE isn't available, so bit 8 is Res1
        static constexpr uint64_t ReservedValues =
            0xFF        | // Reserved [7:0]
            (0b1 << 8)  | // TZ
            (0b1 << 9)  | // Reserved [9]
            (0b11 << 12); // Reserved [13:12]

        // Reserved     [7:0]   (Res1)
        // TZ           [8]     (Res1 if FEAT_SVE is not available)
        // Reserved     [9]     (Res1)
        static constexpr unsigned TFPIndex = 10;
        // Reserved     [11]    (Res0)
        // Reserved     [13:12] (Res1)
        // Reserved     [19:14] (Res0)
        // TTA          [20]
        // Reserved     [29:21] (Res0)
        // TAM          [30]    (Res0 if FEAT_AMUv1 not implemented)
        // TCPAC        [31]
        // Reserved     [63:32] (Res0)

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
        static void Write(HCR_EL2 aValue);

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
        explicit HCR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // VM           [0]
        // SWIO         [1]
        // PTW          [2]
        // FMO          [3]
        // IMO          [4]
        // AMO          [5]
        // VF           [6]
        // VI           [7]
        // VSE          [8]
        // FB           [9]
        // BSU          [11:10]
        // DC           [12]
        // TWI          [13]
        // TWE          [14]
        // TID0         [15]    (Res0 if AArch32 not supported at EL0)
        // TID1         [16]
        // TID2         [17]
        // TID3         [18]
        // TSC          [19]
        // TIDCP        [20]
        // TACR         [21]
        // TSW          [22]
        // TPCP         [23]
        // TPU          [24]
        // TTLB         [25]
        // TVM          [26]
        // TGE          [27]
        // TDZ          [28]
        // HCD          [29]    (Res0 if EL3 not implemented)
        // TRVM         [30]
        static constexpr unsigned RWIndex = 31;
        // CD           [32]
        // ID           [33]
        // E2H          [34]    (Res0 if FEAT_VHE not implemented)
        // TLOR         [35]    (Res0 if FEAT_LOR not implemented)
        // TERR         [36]    (Res0 if FEAT_RAS not implemented)
        // TEA          [37]    (Res0 if FEAT_RAS not implemented)
        // MIOCNCE      [38]
        // Reserved     [39]    (Res0)
        // APK          [40]    (Res0 if FEAT_PAuth not implemented)
        // API          [41]    (Res0 if FEAT_PAuth not implemented)
        // NV           [42]    (Res0 if FEAT_NV2 or FEAT_NV not implemented)
        // NV1          [43]    (Res0 if FEAT_NV2 or FEAT_NV not implemented)
        // AT           [44]    (Res0 if FEAT_NV not implemented)
        // NV2          [45]    (Res0 if FEAT_NV2 not implemented)
        // FWB          [46]    (Res0 if FEAT_S2FWB not implemented)
        // FIEN         [47]    (Res0 if FEAT_RASv1p1 not implemented)
        // Reserved     [48]    (Res0)
        // TID4         [49]    (Res0 if FEAT_EVT not implemented)
        // TICAB        [50]    (Res0 if FEAT_EVT not implemented)
        // AMVOFFEN     [51]    (Res0 if FEAT_AMUv1p1 not implemented)
        // TOCU         [52]    (Res0 if FEAT_EVT not implemented)
        // EnSCXT       [53]    (Res0 if FEAT_CSV2 and FEAT_CSV2_1p2 not implemented)
        // TTLBIS       [54]    (Res0 if FEAT_EVT not implemented)
        // TTLBOS       [55]    (Res0 if FEAT_EVT not implemented)
        // ATA          [56]    (Res0 if FEAT_MTE2 not implemented)
        // DCT          [57]    (Res0 if FEAT_MTE2 not implemented)
        // TID5         [58]    (Res0 if FEAT_MTE2 not implemented)
        // TWEDEn       [59]    (Res0 if FEAT_TWED not implemented)
        // TWEDEL       [63:60] (Res0 if FEAT_TWED not implemented)

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
        static void Write(HSTR_EL2 aValue);

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
        explicit HSTR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // T0           [0]
        // T1           [1]
        // T2           [2]
        // T3           [3]
        // Reserved     [4]     (Res0)
        // T5           [5]
        // T6           [6]
        // T7           [7]
        // T8           [8]
        // T9           [9]
        // T10          [10]
        // T11          [11]
        // T12          [12]
        // T13          [13]
        // Reserved     [14]    (Res0)
        // T15          [15]
        // Reserved     [63:16] (Res0)

        std::bitset<64> RegisterValue;
    };

    /**
     * Memory Attribute Indirection Register
     * https://developer.arm.com/documentation/ddi0595/2020-12/AArch64-Registers/MAIR-EL1--Memory-Attribute-Indirection-Register--EL1-
    */
    class MAIR_EL1
    {
    public:
        /**
         * The number of attributes available
        */
        static constexpr size_t AttributeCount = 8;
        
        /**
         * Constructor - produces a value with all attributes zeroed
        */
        MAIR_EL1() = default;

        /**
         * Writes the given value to the MAIR_EL1 register
         * 
         * @param aValue Value to write
        */
        static void Write(MAIR_EL1 aValue);

        /**
         * Reads the current state of the MAIR_EL1 register
         * 
         * @return The current state of the register
        */
        static MAIR_EL1 Read();

        class Attribute
        {
            friend class MAIR_EL1;
        public:
            /**
             * Obtain an attribute representing normal memory
             * 
             * @return An attribute representing normal memory
            */
            static Attribute NormalMemory();

            /**
             * Obtain an attribute representing device memory
             * 
             * @return An attribute representing device memory
            */
            static Attribute DeviceMemory();

        private:
            /**
             * Creates an attribute with the given value
            */
            explicit Attribute(uint8_t const aValue)
                : Value{ aValue }
            {}

            uint8_t Value = 0;
        };

        /**
         * Sets the attribute for the given index
         * 
         * @param aIndex The attribute to set (0 - AttributeCount)
         * @param aAttribute The value to give it
        */
        void SetAttribute(size_t aIndex, Attribute aValue);

        /**
         * Gets the attribute at the given index
         * 
         * @param aIndex The attribute to get (0 - AttributeCount)
         * @return The attribute in that slot
        */
        Attribute GetAttribute(size_t aIndex) const;

    private:
        explicit MAIR_EL1(uint64_t aRawValue);

        uint8_t Attributes[AttributeCount] = { 0 };
    };

    /**
     * System Control Register (EL1)
     * https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/SCTLR-EL1--System-Control-Register--EL1-
    */
    class SCTLR_EL1
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed (and Res1 bits set)
        */
        SCTLR_EL1()
            : SCTLR_EL1{ ReservedValues }
        {}

        /**
         * Writes the given value to the SCTLR_EL1 register
         * 
         * @param aValue Value to write
        */
        static void Write(SCTLR_EL1 aValue);

        /**
         * Reads the current state of the SCTLR_EL1 register
         * 
         * @return The current state of the register
        */
        static SCTLR_EL1 Read();

        /**
         * M Bit - MMU enable for EL1 & 0
         * 
         * @param aEnableMMU If true, MMU will be enabled for EL1 and 0
        */
        void M(bool const aEnableMMU) { RegisterValue[MIndex] = aEnableMMU; }

        /**
         * RW Bit - Execution state for lower exception levels
         * 
         * @return True if EL1 execution state is AArch64. Otherwise it's AArch32
        */
        bool M() const { return RegisterValue[MIndex]; }

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        explicit SCTLR_EL1(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // We are currently assuming FEAT_SVE isn't available, so bit 8 is Res1
        static constexpr uint64_t ReservedValues = 
            (1 << 7)  | // ITD
            (1 << 8)  | // SED
            (1 << 11) | // EOS
            (1 << 20) | // TSCXT
            (1 << 22) | // EIS
            (1 << 23) | // SPAN
            (1 << 28) | // nTLSMD
            (1 << 29) ; // LSMAOE

        static constexpr unsigned MIndex = 0;
        // A            [1]
        // C            [2]
        // SA           [3]
        // SA0          [4]
        // CP15BEN      [5]     (Res0 if EL0 isn't capable of using AArch32)
        // nAA          [6]     (Res0 if FEAT_LSE2 not implemented)
        // ITD          [7]     (Res1 if EL0 isn't capable of using AArch32)
        // SED          [8]     (Res1 if EL0 isn't capable of using AArch32)
        // UMA          [9]
        // EnRCTX       [10]    (Res0 if FEAT_SPECRES not implemented)
        // EOS          [11]    (Res1 if FEAT_ExS not implemented)
        // I            [12]
        // EnDB         [13]    (Res0 if FEAT_PAuth not implemented)
        // DZE          [14]
        // UCT          [15]
        // nTWI                     [16]
        // Reserved     [17]    (Res0)
        // nTWE         [18]
        // WXN          [19]
        // TSCXT        [20]    (Res1 if FEAT_CSV2_2 and FEAT_CSV2_1p2 not implemented)
        // IESB         [21]    (Res0 if FEAT_IESB not implemented)
        // EIS          [22]    (Res1 if FEAT_ExS not implemented)
        // SPAN         [23]    (Res1 if FEAT_PAN not implemented)
        // EOE          [24]
        // EE           [25]
        // UCI          [26]
        // EnDA         [27]    (Res0 if FEAT_PAuth not implemented)
        // nTLSMD       [28]    (Res1 if FEAT_LSMAOC not implemented)
        // LSMAOE       [29]    (Res1 if FEAT_LSMAOC not implemented)
        // EnIB         [30]    (Res0 if FEAT_PAuth not implemented)
        // EnIA         [31]    (Res0 if FEAT_PAuth not implemented)
        // Reserved     [34:32] (Res0)
        // BT0          [35]    (Res0 if FEAT_BTI not implemented)
        // BT1          [36]    (Res0 if FEAT_BTI not implemented)
        // ITFSB        [37]    (Res0 if FEAT_MTE2 not implemented)
        // TCF0         [39:38] (Res0 if FEAT_MTE not implemented)
        // TCF          [41:40] (Res0 if FEAT_MTE not implemented)
        // ATA0         [42]    (Res0 if FEAT_MTE2 not implemented)
        // ATA          [43]    (Res0 if FEAT_MTE2 not implemented)
        // DSSBS        [44]    (Res0 if FEAT_SSBS not implemented)
        // TWEDEn       [45]    (Res0 if FEAT_TWED not implemented)
        // TWEDEL       [49:46] (Res0 if FEAT_TWED not implemented)
        // Reserved     [53:50] (Res0)
        // EnASR        [54]    (Res0 if FEAT_LS64 not implemented)
        // EnAS0        [55]    (Res0 if FEAT_LS64 not implemented)
        // EnALS        [56]    (Res0 if FEAT_LS64 not implemented)
        // EPAN         [57]    (Res0 if FEAT_PAN3 not implemented)
        // Reserved     [63:58] (Res0)

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
        static void Write(SPSR_EL2 aValue);

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
        void M(Mode aMode);

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
        explicit SPSR_EL2(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        static constexpr unsigned MIndex_Shift = 0; // bits [3:0]
        static constexpr uint64_t MIndex_Mask = 0b1111;
        // M[4]         [4]
        // Reserved     [5] (Res0)
        static constexpr unsigned FIndex = 6;
        static constexpr unsigned IIndex = 7;
        static constexpr unsigned AIndex = 8;
        static constexpr unsigned DIndex = 9;
        // BTYPE        [11:10] (Res0 if FEAT_BTI not implemented)
        // SSBS         [12]    (Res0 if FEAT_SSBS not implemented)
        // ALLINT       [13]    (Res0 if FEAT_NMI not implemented)
        // Reserved     [19:14] (Res0)
        // IL           [20]
        // SS           [21]
        // PAN          [22]    (Res0 if FEAT_PAN not implemented)
        // UAO          [23]    (Res0 if FEAT_UAO not implemented)
        // DIT          [24]    (Res0 if FEAT_DIT not implemented)
        // TCO          [25]    (Res0 if FEAT_MTE not implemented)
        // Reserved     [27:26] (Res0)
        // V            [28]
        // C            [29]
        // Z            [30]
        // N            [31]
        // PM           [32]    (Res0 if FEAT_EBEP not implemented)
        // PPEND        [33]    (Res0 if FEAT_SEBEP not implemented)
        // EXLOCK       [34]    (Res0 if FEAT_GCS not implemented)
        // PACM         [35]    (Res0 if FEAT_PAuth_LR not implemented)
        // Reserved:    [63:36] (Res0)

        std::bitset<64> RegisterValue;
    };

    /**
     * Translation Control Register (EL1)
     * https://developer.arm.com/documentation/ddi0595/2021-09/AArch64-Registers/TCR-EL1--Translation-Control-Register--EL1-
    */
    class TCR_EL1
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed
        */
        TCR_EL1() = default;

        /**
         * Writes the given value to the TCR_EL1 register
         * 
         * @param aValue Value to write
        */
        static void Write(TCR_EL1 aValue);

        /**
         * Reads the current state of the TCR_EL1 register
         * 
         * @return The current state of the register
        */
        static TCR_EL1 Read();

        /**
         * T0SZ bits - controls the size of the memory region addressed by TTBR0_EL1
         * 
         * @param aBits the number of bits in the user region that can be used
        */
        void T0SZ(uint8_t aBits);

        /**
         * T0SZ bits - controls the size of the memory region addressed by TTBR0_EL1
         * 
         * @return The number of bits in the user region that can be used
        */
        uint8_t T0SZ() const;

        enum class T0Granule: uint8_t
        {
            Size4kb = 0b00,
            Size64kb = 0b01,
            Size16kb = 0b10,
        };

        /**
         * TG0 bits - controls the granule size of TTBR0_EL1
         * 
         * @param aSize The granule size for the user region
        */
        void TG0(T0Granule aSize);

        /**
         * TG0 bits - controls the granule size of TTBR0_EL1
         * 
         * @return The granule size for the user region
        */
        T0Granule TG0() const;

        /**
         * T1SZ bits - controls the size of the memory region addressed by TTBR1_EL1
         * 
         * @param aBits The number of bits in the kernel region that can be used
        */
        void T1SZ(uint8_t aBits);

        /**
         * T1SZ bits - controls the size of the memory region addressed by TTBR1_EL1
         * 
         * @return The number of bits in the kernel region that can be used
        */
        uint8_t T1SZ() const;

        enum class T1Granule: uint8_t
        {
            Size16kb = 0b01,
            Size4kb = 0b10,
            Size64kb = 0b11,
        };

        /**
         * TG1 bits - controls the granule size of TTBR1_EL1
         * 
         * @param aSize The granule size for the kernel region
        */
        void TG1(T1Granule aSize);

        /**
         * TG1 bits - controls the granule size of TTBR1_EL1
         * 
         * @return The granule size for the kernel region
        */
        T1Granule TG1() const;

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        explicit TCR_EL1(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        static constexpr unsigned T0SZIndex_Shift = 0; // bits [5:0]
        static constexpr uint64_t T0SZIndex_Mask = 0b11'1111;
        // Reserved     [6]     (Res0)
        // EPD0         [7]
        // IRGN0        [9:8]
        // ORGN0        [11:10]
        // SH0          [13:12]
        static constexpr unsigned TG0Index_Shift = 14; // bits [15:14]
        static constexpr uint64_t TG0Index_Mask = 0b11;
        static constexpr unsigned T1SZIndex_Shift = 16; // bits [21:16]
        static constexpr uint64_t T1SZIndex_Mask = 0b11'1111;
        // A1           [22]
        // EPD1         [23]
        // IRGN1        [25:24]
        // ORGN1        [27:26]
        // SH1          [29:28]
        static constexpr unsigned TG1Index_Shift = 30; // bits [31:30]
        static constexpr uint64_t TG1Index_Mask = 0b11;
        // IPS          [34:32]
        // Reserved     [35]    (Res0)
        // AS           [36]
        // TBA0         [37]
        // TBA1         [38]
        // HA           [39]    (Res0 if FEAT_HAFDBS not implemented)
        // HD           [40]    (Res0 if FEAT_HAFDBS not implemented)
        // HPD0         [41]    (Res0 if FEAT_HPDS not implemented)
        // HPD1         [42]    (Res0 if FEAT_HPDS not implemented)
        // HWU059       [43]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU060       [44]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU061       [45]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU062       [46]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU159       [47]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU160       [48]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU161       [49]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // HWU162       [50]    (RAZ/WI if FEAT_HPDS2 not implemented)
        // TBID0        [51]    (Res0 if FEAT_PAuth not implemented)
        // TBID1        [52]    (Res0 if FEAT_PAuth not implemented)
        // NFD0         [53]    (Res0 if FEAT_SVE not implemented)
        // NFD1         [54]    (Res0 if FEAT_SVE not implemented)
        // E0PD0        [55]    (Res0 if FEAT_E0PD not implemented)
        // E0PD1        [56]    (Res0 if FEAT_E0PD not implemented)
        // TCMA0        [57]    (Res0 if FEAT_MTE2 not implemented)
        // TCMA1        [58]    (Res0 if FEAT_MTE2 not implemented)
        // DS           [59]    (Res0 if FEAT_LPA2 not implemented)
        // Reserved     [63:60] (Res0)

        std::bitset<64> RegisterValue;
    };

    /**
     * Translation Table Base Register (0/1) (EL1)
     * https://developer.arm.com/documentation/ddi0595/2021-09/AArch64-Registers/TTBR0-EL1--Translation-Table-Base-Register-0--EL1-
     * https://developer.arm.com/documentation/ddi0595/2021-09/AArch64-Registers/TTBR1-EL1--Translation-Table-Base-Register-1--EL1-
    */
    class TTBRn_EL1
    {
        static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Need to adjust which value is used to retrieve the bitset");
    public:
        /**
         * Constructor - produces a value with all bits zeroed
        */
        TTBRn_EL1() = default;

        /**
         * Writes the given value to the TTBR0_EL1 register
         * 
         * @param aValue Value to write
        */
        static void Write0(TTBRn_EL1 aValue);

        /**
         * Writes the given value to the TTBR1_EL1 register
         * 
         * @param aValue Value to write
        */
        static void Write1(TTBRn_EL1 aValue);

        /**
         * Reads the current state of the TTBR0_EL1 register
         * 
         * @return The current state of the register
        */
        static TTBRn_EL1 Read0();

        /**
         * Reads the current state of the TTBR1_EL1 register
         * 
         * @return The current state of the register
        */
        static TTBRn_EL1 Read1();

        /**
         * BADDR bits - Translation table base address
         * 
         * @param aBaseAddress The table base address
        */
        void BADDR(uintptr_t aBaseAddress);

        /**
         * BADDR bits - Translation table base address
         * 
         * @return The table base address
        */
        uintptr_t BADDR() const;

    private:
        /**
         * Create a register value from the given bits
         * 
         * @param aInitialValue The bits to start with
        */
        explicit TTBRn_EL1(uint64_t const aInitialValue)
            : RegisterValue{ aInitialValue }
        {}

        // CnP          [0]
        // not shifting because the data isn't shifted when stored, we just mask off the top and bottom bits
        static constexpr unsigned BADDRIndex_Shift = 0; // bits [47:1]
        static constexpr uint64_t BADDRIndex_Mask = 0x0000'FFFF'FFFF'FFFEULL;
        // ASID         [63:48] (if implementation only supports 8 bits of ASID, then the top 8 bits are Res0)

        // Sanity check to make sure we're masking what we think we are
        static_assert((0xFFFF'0000'0000'0000ULL & (BADDRIndex_Mask << BADDRIndex_Shift) & 1) == 0, "Bitfields overlap");

        std::bitset<64> RegisterValue;
    };
}

#endif // KERNEL_AARCH64_SYSTEM_REGISTERS_H