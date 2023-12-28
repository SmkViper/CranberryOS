#include "MemoryDescriptorTests.h"

#include "../../AArch64/MemoryDescriptor.h"
#include "../Framework.h"

namespace UnitTests::AArch64::MemoryDescriptor
{
    namespace Details
    {
        /**
         * Helper that the descriptors designate as a friend so we can validate the bit patterns
         */
        struct TestAccessor
        {
            /**
             * Get the descriptor value of a fault descriptor
             * 
             * @param aDescriptor The descriptor to extract the bits from
             * @return The bits in the descriptor
             */
            template<typename DescriptorT>
            static uint64_t GetDescriptorValue(DescriptorT const aDescriptor)
            {
                if constexpr (std::is_same_v<decltype(DescriptorT::DescriptorBits), uint64_t>)
                {
                    return aDescriptor.DescriptorBits;
                }
                else
                {
                    return aDescriptor.DescriptorBits.to_ullong();
                }
            }
        };
    }

    namespace
    {
        /**
         * Tests on the Fault descriptor
         */
        void FaultDescriptorTest()
        {
            ::AArch64::Descriptor::Fault testDescriptor;

            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0b00, "Fault descriptor construction");
            EmitTestResult(::AArch64::Descriptor::Fault::IsType(0b00)
                && !::AArch64::Descriptor::Fault::IsType(0b01)
                && !::AArch64::Descriptor::Fault::IsType(0b10)
                && !::AArch64::Descriptor::Fault::IsType(0b11)
                , "Fault descriptor IsType with just type bits");
            EmitTestResult(::AArch64::Descriptor::Fault::IsType(0b1100), "Fault descriptor with non type bits");
        }

        /**
         * Tests on the table descriptor
         */
        void TableDescriptorTest()
        {
            ::AArch64::Descriptor::Table testDescriptor;

            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0b11, "Table descriptor construction");
            EmitTestResult(!::AArch64::Descriptor::Table::IsType(0b00)
                && !::AArch64::Descriptor::Table::IsType(0b01)
                && !::AArch64::Descriptor::Table::IsType(0b10)
                && ::AArch64::Descriptor::Table::IsType(0b11)
                , "Table descriptor descriptor IsType with just type bits");
            EmitTestResult(::AArch64::Descriptor::Table::IsType(0b1111), "Table descriptor with non type bits");

            testDescriptor.Address(0xFEFE'FEFE'FEFE'FEFE);
            auto const filteredAddress = testDescriptor.Address();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0000'FEFE'FEFE'F003
                && filteredAddress == 0x0000'FEFE'FEFE'F000
                , "Table descriptor Address get/set");
            
            uint64_t buffer[3] = {};
            ::AArch64::Descriptor::Table::Write(testDescriptor, buffer, 1);
            EmitTestResult(buffer[0] == 0
                && buffer[1] == Details::TestAccessor::GetDescriptorValue(testDescriptor)
                && buffer[2] == 0
                , "Table descriptor Write");
        }
    }

    void Run()
    {
        FaultDescriptorTest();
        TableDescriptorTest();
    }
}