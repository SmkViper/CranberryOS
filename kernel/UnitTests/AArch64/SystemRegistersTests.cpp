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
    }
}