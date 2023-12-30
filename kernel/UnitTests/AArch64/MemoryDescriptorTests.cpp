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
            auto const readAddress = testDescriptor.Address();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0000'FEFE'FEFE'F003
                && readAddress == 0x0000'FEFE'FEFE'F000
                , "Table descriptor Address get/set");
            
            uint64_t buffer[3] = {};
            ::AArch64::Descriptor::Table::Write(testDescriptor, buffer, 1);
            EmitTestResult(buffer[0] == 0
                && buffer[1] == Details::TestAccessor::GetDescriptorValue(testDescriptor)
                && buffer[2] == 0
                , "Table descriptor Write");
        }

        /**
         * Tests on the block descriptors
         * 
         * @param apBlockTypeName The block type name for use in test message reporting
         */
        template<uint64_t AddressMask>
        void BlockDescriptorTest(char const* const apBlockTypeName)
        {
            using BlockT = ::AArch64::Descriptor::Details::BlockT<AddressMask>;
            BlockT testDescriptor;

            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0b01, "Block {} descriptor construction", apBlockTypeName);
            EmitTestResult(!BlockT::IsType(0b00)
                && BlockT::IsType(0b01)
                && !BlockT::IsType(0b10)
                && !BlockT::IsType(0b11)
                , "Block {} descriptor descriptor IsType with just type bits", apBlockTypeName);
            EmitTestResult(BlockT::IsType(0b1101), "Block {} descriptor with non type bits", apBlockTypeName);

            auto const rawAddress = 0xFEFE'FEFE'FEFE'FEFEull;
            testDescriptor.Address(rawAddress);
            auto const readAddress = testDescriptor.Address();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == ((rawAddress & AddressMask) | 0b01)
                && readAddress == (rawAddress & AddressMask)
                , "Block {} descriptor Address get/set", apBlockTypeName);
            
            auto prevDescriptorValue = Details::TestAccessor::GetDescriptorValue(testDescriptor);

            // AttrIndx [4:2]
            auto const rawAttrIndex = 0b101;
            testDescriptor.AttrIndx(rawAttrIndex);
            auto const readRawIndex = testDescriptor.AttrIndx();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == (prevDescriptorValue | (rawAttrIndex << 2))
                && readRawIndex == rawAttrIndex
                , "Block {} descriptor AttrIndx get/set", apBlockTypeName);

            prevDescriptorValue = Details::TestAccessor::GetDescriptorValue(testDescriptor);
            
            // AP [7:6]
            auto const rawAP = BlockT::AccessPermissions::KernelROUserRO; // 0b11
            testDescriptor.AP(rawAP);
            auto const readAP = testDescriptor.AP();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == (prevDescriptorValue | (static_cast<uint32_t>(rawAP) << 6))
                && rawAP == readAP
                , "Block {} descriptor AP get/set", apBlockTypeName);

            prevDescriptorValue = Details::TestAccessor::GetDescriptorValue(testDescriptor);

            // AF [10]
            auto const rawAF = true;
            testDescriptor.AF(rawAF);
            auto const readAF = testDescriptor.AF();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == (prevDescriptorValue | (static_cast<uint32_t>(rawAF) << 10))
                && rawAF == readAF
                , "Block {} descriptor AF get/set", apBlockTypeName);
            
            uint64_t buffer[3] = {};
            BlockT::Write(testDescriptor, buffer, 1);
            EmitTestResult(buffer[0] == 0
                && buffer[1] == Details::TestAccessor::GetDescriptorValue(testDescriptor)
                && buffer[2] == 0
                , "Block {} descriptor Write", apBlockTypeName);
        }

        /**
         * Tests on page descriptors
         */
        void PageDescriptorTest()
        {
            ::AArch64::Descriptor::Page testDescriptor;

            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0b11, "Page descriptor construction");
            EmitTestResult(!::AArch64::Descriptor::Page::IsType(0b00)
                && !::AArch64::Descriptor::Page::IsType(0b01)
                && !::AArch64::Descriptor::Page::IsType(0b10)
                && ::AArch64::Descriptor::Page::IsType(0b11)
                , "Page descriptor descriptor IsType with just type bits");
            EmitTestResult(::AArch64::Descriptor::Page::IsType(0b1111), "Page descriptor with non type bits");

            testDescriptor.Address(PhysicalPtr{ 0xFEFE'FEFE'FEFE'FEFE });
            auto const readAddress = testDescriptor.Address();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0000'FEFE'FEFE'F003
                && readAddress == PhysicalPtr{ 0x0000'FEFE'FEFE'F000 }
                , "Page descriptor Address get/set");

            // AttrIndx [4:2]
            auto const rawAttrIndex = 0b101;
            testDescriptor.AttrIndx(rawAttrIndex);
            auto const readRawIndex = testDescriptor.AttrIndx();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0000'FEFE'FEFE'F017
                && readRawIndex == rawAttrIndex
                , "Page descriptor AttrIndx get/set");
            
            // AP [7:6]
            auto const rawAP = ::AArch64::Descriptor::Page::AccessPermissions::KernelROUserRO; // 0b11
            testDescriptor.AP(rawAP);
            auto const readAP = testDescriptor.AP();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0000'FEFE'FEFE'F0D7
                && rawAP == readAP
                , "Page descriptor AP get/set");
            
            // AF [10]
            auto const rawAF = true;
            testDescriptor.AF(rawAF);
            auto const readAF = testDescriptor.AF();
            EmitTestResult(Details::TestAccessor::GetDescriptorValue(testDescriptor) == 0x0000'FEFE'FEFE'F4D7
                && rawAF == readAF
                , "Page descriptor AF get/set");
            
            uint64_t buffer[3] = {};
            ::AArch64::Descriptor::Page::Write(testDescriptor, buffer, 1);
            EmitTestResult(buffer[0] == 0
                && buffer[1] == Details::TestAccessor::GetDescriptorValue(testDescriptor)
                && buffer[2] == 0
                , "Page descriptor Write");
        }
    }

    void Run()
    {
        FaultDescriptorTest();
        TableDescriptorTest();
        BlockDescriptorTest<::AArch64::Descriptor::Details::L1Address_Mask>("L1");
        BlockDescriptorTest<::AArch64::Descriptor::Details::L2Address_Mask>("L2");
        PageDescriptorTest();
    }
}