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
            static uint64_t GetDescriptorValue(::AArch64::Descriptor::Fault const aDescriptor)
            {
                return aDescriptor.DescriptorBits;
            }
        };
    }

    namespace
    {
        void FaultDescriptorTest()
        {
            ::AArch64::Descriptor::Fault testDescriptor;

            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0, "Fault descriptor");
            EmitTestResult(::AArch64::Descriptor::Fault::IsType(0b00)
                && !::AArch64::Descriptor::Fault::IsType(0b01)
                && !::AArch64::Descriptor::Fault::IsType(0b10)
                && !::AArch64::Descriptor::Fault::IsType(0b11)
                , "Fault descriptor type check with just type bits");
            EmitTestResult(::AArch64::Descriptor::Fault::IsType(0b1100), "Fault descriptor with non type bits");
        }
    }

    void Run()
    {
        FaultDescriptorTest();
    }
}