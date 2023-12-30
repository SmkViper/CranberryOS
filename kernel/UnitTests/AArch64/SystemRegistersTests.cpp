#include "SystemRegistersTests.h"

#include <cstring>
#include "../../AArch64/SystemRegisters.h"
#include "../Framework.h"

namespace UnitTests::AArch64::SystemRegisters
{
    namespace Details
    {
        /**
         * Helper that the registers designate as a friend so we can validate the bit patterns
         */
        struct TestAccessor
        {
            /**
             * Get the bit value of a register wrapper
             * 
             * @param aRegister The register to extract the bits from
             * @return The bits in the register
             */
            template<typename RegisterT>
            static uint64_t GetRegisterValue(RegisterT const aRegister)
            {
                if constexpr (std::is_same_v<decltype(RegisterT::RegisterValue), uint64_t>)
                {
                    return aRegister.RegisterValue;
                }
                else
                {
                    return aRegister.RegisterValue.to_ullong();
                }
            }

            /**
             * Get the bit value of a register wrapper
             * 
             * @param aRegister The register to extract the bits from
             * @return The bits in the register
             */
            template<>
            uint64_t GetRegisterValue(::AArch64::MAIR_EL1 const aRegister)
            {
                // We're specializing the template for MAIR_EL1 since it stores values in an array
                uint64_t retVal = 0;
                static_assert(sizeof(retVal) == sizeof(aRegister.Attributes), "Unexpected size difference");
                memcpy(&retVal, aRegister.Attributes, sizeof(retVal));
                return retVal;
            }
        };
    }

    namespace
    {
        // NOTE: Read functions are tested by hand-writing the reading of the register and comparing it to the result
        // of the Read() function. This isn't great, but considering the values in these registers will be changing as
        // development continues, I'm not sure of a better way to do it. At least by hand-writing the code in the tests
        // the hope is that any typos will be caught (i.e. if Read is reading the wrong register).

        /**
         * Test the CPACR_EL1 register wrapper
         */
        void CPACR_EL1Test()
        {
            ::AArch64::CPACR_EL1 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "CPACR_EL1 default value");

            // FPEN [20:21]
            testRegister.FPEN(::AArch64::CPACR_EL1::FPENTraps::TrapNone); // 0b11
            auto const readFPEN = testRegister.FPEN();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x0030'0000
                && readFPEN == ::AArch64::CPACR_EL1::FPENTraps::TrapNone
                , "CPACR_EL1 FPEN get/set");
            
            // Write not tested as it affects system operation

            uint64_t readRawValue = 0;
            asm volatile(
                "mrs %[value], cpacr_el1"
                :[value] "=r"(readRawValue) // outputs
                : // no inputs
                : // no bashed registers
            );
            EmitTestResult(Details::TestAccessor::GetRegisterValue(::AArch64::CPACR_EL1::Read()) == readRawValue, "CPACR_EL1 read");
        }

        /**
         * Test the CPTR_EL2 register wrapper
         */
        void CPTR_EL2Test()
        {
            ::AArch64::CPTR_EL2 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x33FF, "CPTR_EL2 default value");

            // TFP [10]
            testRegister.TFP(true);
            auto const readTFP = testRegister.TFP();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x37FF
                && readTFP == true
                , "CPTR_EL2 TFP get/set");
            
            // Read/Write not tested as we're running in EL1, and it can only be read/written in EL2
        }

        /**
         * Test the HCR_EL2 register wrapper
         */
        void HCR_EL2Test()
        {
            ::AArch64::HCR_EL2 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "HCR_EL2 default value");

            // RW [31]
            testRegister.RW(true);
            auto const readRW = testRegister.RW();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x8000'0000
                && readRW == true
                , "HCR_EL2 RW get/set");
            
            // Read/Write not tested as we're running in EL1, and it can only be read/written in EL2
        }

        /**
         * Test the HSTR_EL2 register wrapper
         */
        void HSTR_EL2Test()
        {
            ::AArch64::HSTR_EL2 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "HSTR_EL2 default value");

            // Read/Write not tested as we're running in EL1, and it can only be read/written in EL2
        }

        /**
         * Test the MAIR_EL1 Attribute wrapper
         */
        void MAIR_EL1AttributeTest()
        {
            auto const normal = ::AArch64::MAIR_EL1::Attribute::NormalMemory();
            auto const device = ::AArch64::MAIR_EL1::Attribute::DeviceMemory();
            EmitTestResult((normal == normal) && !(normal == device), "MAIR_EL1 Attribute ==");
            EmitTestResult(!(normal != normal) && (normal != device), "MAIR_EL1 Attribute !=");
        }

        /**
         * Test the MAIR_EL1 register wrapper
         */
        void MAIR_EL1Test()
        {
            ::AArch64::MAIR_EL1 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "MAIR_EL1 default value");

            // Attributes are stored as 8 bit values, essentially as an array
            auto const normalMemoryAttribute = ::AArch64::MAIR_EL1::Attribute::NormalMemory(); // 0x44
            auto testAttribute = [&testRegister, normalMemoryAttribute](size_t const aIndex, uint64_t const aExpectedValue)
            {
                testRegister.SetAttribute(aIndex, normalMemoryAttribute);
                auto const readAttribute = testRegister.GetAttribute(aIndex);
                EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == aExpectedValue
                    && readAttribute == normalMemoryAttribute
                    , "MAIR_EL1 Attribute {} get/set", aIndex);
            };
            
            auto expectedRegisterValue = 0ull;
            for (auto curIndex = 0u; curIndex < ::AArch64::MAIR_EL1::AttributeCount; ++curIndex)
            {
                expectedRegisterValue |= (0x44ull << (curIndex * 8));
                testAttribute(curIndex, expectedRegisterValue);
            }
            
            // Write not tested as it affects system operation

            uint64_t readRawValue = 0;
            asm volatile(
                "mrs %[value], mair_el1"
                :[value] "=r"(readRawValue) // outputs
                : // no inputs
                : // no bashed registers
            );
            EmitTestResult(Details::TestAccessor::GetRegisterValue(::AArch64::MAIR_EL1::Read()) == readRawValue, "MAIR_EL1 read");
        }

        /**
         * Test the SCTLR_EL1 register wrapper
         */
        void SCTLR_EL1Test()
        {
            ::AArch64::SCTLR_EL1 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x30D0'0980, "SCTLR_EL1 default value");

            // M [0]
            testRegister.M(true);
            auto const readM = testRegister.M();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x30D0'0981
                && readM == true
                , "SCTLR_EL1 M get/set");
            
            // Write not tested as it affects system operation

            uint64_t readRawValue = 0;
            asm volatile(
                "mrs %[value], sctlr_el1"
                :[value] "=r"(readRawValue) // outputs
                : // no inputs
                : // no bashed registers
            );
            EmitTestResult(Details::TestAccessor::GetRegisterValue(::AArch64::SCTLR_EL1::Read()) == readRawValue, "SCTLR_EL1 read");
        }

        /**
         * Test the SPSR_EL2 register wrapper
         */
        void SPSR_EL2Test()
        {
            ::AArch64::SPSR_EL2 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "SPSR_EL2 default value");

            // M [3:0]
            testRegister.M(::AArch64::SPSR_EL2::Mode::EL2h); // 0b1001
            auto const readM = testRegister.M();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x0009
                && readM == ::AArch64::SPSR_EL2::Mode::EL2h
                , "SPSR_EL2 M get/set");
            
            // F [6]
            testRegister.F(true);
            auto const readF = testRegister.F();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x0049
                && readF == true
                , "SPSR_EL2 F get/set");
            
            // I [7]
            testRegister.I(true);
            auto const readI = testRegister.I();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x00C9
                && readI == true
                , "SPSR_EL2 I get/set");

            // A [8]
            testRegister.A(true);
            auto const readA = testRegister.A();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x01C9
                && readA == true
                , "SPSR_EL2 A get/set");

            // D [9]
            testRegister.D(true);
            auto const readD = testRegister.D();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x03C9
                && readD == true
                , "SPSR_EL2 D get/set");
            
            // Read/Write not tested as we're running in EL1, and it can only be read/written in EL2
        }

        /**
         * Test the TCR_EL1 register wrapper
         */
        void TCR_EL1Test()
        {
            ::AArch64::TCR_EL1 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "TCR_EL1 default value");

            // NOTE: T0SZ and T1SZ take/return the bit count for address space, but store it internally as the number
            // most significant bits that are used to pick between kernal and user space. So if, for example, we want
            // 48 bits of address space, it will store 16 (the number of high bits in 64-bit addresses). So our tests
            // need to account for this in the raw comparison

            // T0SZ [5:0]
            testRegister.T0SZ(0b1'0111); // stored as 64 - 0b1'0111 = 0b10'1001
            auto const readT0SZ = testRegister.T0SZ();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x0029
                && readT0SZ == 0b1'0111
                , "TCR_EL1 T0SZ get/set");
            
            // TG0 [15:14]
            testRegister.TG0(::AArch64::TCR_EL1::T0Granule::Size16kb); // 0b10
            auto const readTG0 = testRegister.TG0();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x8029
                && readTG0 == ::AArch64::TCR_EL1::T0Granule::Size16kb
                , "TCR_EL1 TG0 get/set");

            // T1SZ [21:16]
            testRegister.T1SZ(0b0111); // stored as 64 - 0b0111 = 0b11'1001
            auto const readT1SZ = testRegister.T1SZ();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x39'8029
                && readT1SZ == 0b0111
                , "TCR_EL1 T1SZ get/set");
            
            // TG1 [31:30]
            testRegister.TG1(::AArch64::TCR_EL1::T1Granule::Size64kb); // 0b11
            auto const readTG1 = testRegister.TG1();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0xC039'8029
                && readTG1 == ::AArch64::TCR_EL1::T1Granule::Size64kb
                , "TCR_EL1 TG1 get/set");
            
            // Write not tested as it affects system operation

            uint64_t readRawValue = 0;
            asm volatile(
                "mrs %[value], tcr_el1"
                :[value] "=r"(readRawValue) // outputs
                : // no inputs
                : // no bashed registers
            );
            EmitTestResult(Details::TestAccessor::GetRegisterValue(::AArch64::TCR_EL1::Read()) == readRawValue, "TCR_EL1 read");
        }

        /**
         * Test the TTBRn_EL1 register wrapper
         */
        void TTBRn_EL1Test()
        {
            ::AArch64::TTBRn_EL1 testRegister;
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0, "TTBRn_EL1 default value");

            // BADDR [47:1]
            testRegister.BADDR(PhysicalPtr{ 0xAAAA'AAAA'AAAA'AAA5 });
            auto const readT0SZ = testRegister.BADDR();
            EmitTestResult(Details::TestAccessor::GetRegisterValue(testRegister) == 0x0000'AAAA'AAAA'AAA4
                && readT0SZ == PhysicalPtr{ 0x0000'AAAA'AAAA'AAA4 } // top bits and bottom bit get masked off
                , "TTBRn_EL1 BADDR get/set");
            
            // Write0/1 not tested as it affects system operation

            uint64_t readRawValue = 0;
            asm volatile(
                "mrs %[value], ttbr0_el1"
                :[value] "=r"(readRawValue) // outputs
                : // no inputs
                : // no bashed registers
            );
            EmitTestResult(Details::TestAccessor::GetRegisterValue(::AArch64::TTBRn_EL1::Read0()) == readRawValue, "TTBRn_EL1 read 0");

            asm volatile(
                "mrs %[value], ttbr1_el1"
                :[value] "=r"(readRawValue) // outputs
                : // no inputs
                : // no bashed registers
            );
            EmitTestResult(Details::TestAccessor::GetRegisterValue(::AArch64::TTBRn_EL1::Read1()) == readRawValue, "TTBRn_EL1 read 1");
        }
    }

    void Run()
    {
        CPACR_EL1Test();
        CPTR_EL2Test();
        HCR_EL2Test();
        HSTR_EL2Test();
        MAIR_EL1AttributeTest();
        MAIR_EL1Test();
        SCTLR_EL1Test();
        SPSR_EL2Test();
        TCR_EL1Test();
        TTBRn_EL1Test();
    }
}