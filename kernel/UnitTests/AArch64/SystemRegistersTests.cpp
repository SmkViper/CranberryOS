#include "SystemRegistersTests.h"

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
        };
    }

    namespace
    {
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
    }

    void Run()
    {
        CPACR_EL1Test();
        CPTR_EL2Test();
    }
}