#ifndef KERNEL_AARCH64_SYSTEM_REGISTERS_H
#define KERNEL_AARCH64_SYSTEM_REGISTERS_H

#include <bitset>
#include <cstdint>

namespace AArch64
{
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
        static void Write(HCR_EL2 const aValue)
        {
            uint64_t const rawValue = aValue.RegisterValue.to_ulong();
            asm volatile(
                "msr hcr_el2, %[value]"
                : // no outputs
                :[value] "r"(rawValue) // inputs
                : // no bashed registers
            );
        }

        /**
         * Reads the current state of the HCR_EL2 register
         * 
         * @return The current state of the register
        */
        static HCR_EL2 Read()
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

        /**
         * RW Bit - Execution state for lower exception levels
         * 
         * @param aEL1ExecutionIsAArch64 If true, EL1 will be AArch64, otherwise it will be AArch32
        */
        void RW(bool aEL1ExecutionIsAArch64) { RegisterValue[RWIndex] = aEL1ExecutionIsAArch64; }

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
}

#endif // KERNEL_AARCH64_SYSTEM_REGISTERS_H