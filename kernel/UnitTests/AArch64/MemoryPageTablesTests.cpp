#include "MemoryPageTablesTests.h"

#include "../../AArch64/MemoryPageTables.h"
#include "../Framework.h"

namespace UnitTests::AArch64::MemoryPageTables
{
    namespace Details
    {
        /**
         * Helper that the table entries designate as a friend so we can validate their logic
         */
        struct TestAccessor
        {
            /**
             * Build the given entry from the bits
             * 
             * @tparam EntryT The type of entry to construct
             * @param aBits The bits to construct from
             * 
             * @return The constructed entry
             */
            template<typename EntryT>
            static EntryT BuildEntry(uint64_t const aBits)
            {
                return EntryT{ aBits, ::AArch64::PageTable::Details::EntryConstructTag{} };
            }

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
        // Entry tests are testing the Details::TestAndVisit and Details::VisitImpl functions as well

        /**
         * Test the Details::Entry type
         */
        void EntryTest()
        {
            // only difference between the entries are the descriptor types they support, so just test one
            auto const testFaultEntry = Details::TestAccessor::BuildEntry<::AArch64::PageTable::Level0View::Entry>(0b1100 /* fault type 0b00 */);

            auto faultVisitFault = false;
            auto faultVisitTable = false;
            testFaultEntry.Visit(Overloaded{
                [&faultVisitFault](::AArch64::Descriptor::Fault) { faultVisitFault = true; },
                [&faultVisitTable](::AArch64::Descriptor::Table) { faultVisitTable = true; }
            });

            auto const testTableEntry = Details::TestAccessor::BuildEntry<::AArch64::PageTable::Level0View::Entry>(0b1111 /* table type 0b11 */);

            auto tableVisitFault = false;
            auto tableVisitTable = false;
            auto tableBits = 0ull;
            testTableEntry.Visit(Overloaded{
                [&tableVisitFault](::AArch64::Descriptor::Fault) { tableVisitFault = true; },
                [&tableVisitTable, &tableBits](::AArch64::Descriptor::Table const aDescriptor)
                {
                    tableVisitTable = true;
                    tableBits = Details::TestAccessor::GetDescriptorValue(aDescriptor);
                }
            });

            EmitTestResult(faultVisitFault && !faultVisitTable, "Entry Visit for fault");
            EmitTestResult(!tableVisitFault && tableVisitTable
                && (tableBits == 0b1111)
                , "Entry Visit for table");
        }

        /**
         * Test the various page view types
         * 
         * @tparam PageViewT The type of page view to test
         * @tparam DescriptorT The descriptor read/write from the view
         * @param apPageViewName The name of the page view for test output
         * @param aVirtualAddress The virtual address to test with
         * @param aExpectedTableIndex The expected table index to be written to/read from
         */
        template<typename PageViewT, typename DescriptorT>
        void PageViewTest(char const* const apPageViewName, VirtualPtr const aVirtualAddress, uint8_t const aExpectedTableIndex)
        {
            uint64_t buffer[10] = {}; // don't need a full table, but need enough of one to differentiate between types
            PageViewT testPageView{ buffer };

            EmitTestResult(testPageView.GetTablePtr() == buffer, "Page view {} construction and VA access", apPageViewName);

            DescriptorT descriptor;
            auto const descriptorValue = Details::TestAccessor::GetDescriptorValue(descriptor);
            testPageView.SetEntryForVA(aVirtualAddress, descriptor);
            
            EmitTestResult(buffer[aExpectedTableIndex] == descriptorValue, "Page view {} SetEntryForVA", apPageViewName);

            auto const entry = testPageView.GetEntryForVA(aVirtualAddress);
            entry.Visit([descriptorValue, apPageViewName](auto aDescriptor)
                {
                    if constexpr (std::is_same_v<std::remove_cv_t<decltype(aDescriptor)>, DescriptorT>)
                    {
                        auto const bits = Details::TestAccessor::GetDescriptorValue(aDescriptor);
                        EmitTestResult(bits == descriptorValue, "Page view {} GetEntryForVA", apPageViewName);
                    }
                    else
                    {
                        EmitTestResult(false, "Page view {} GetEntryForVA (wrong type)", apPageViewName);
                    }
                }
            );
        }
    }

    void Run()
    {
        EntryTest();
        
        auto const l3Index = 1;
        auto const l2Index = 2;
        auto const l1Index = 3;
        auto const l0Index = 4;
        // This address is specifically crafted to pick a different index based on the page table level
        auto const virtualAddress = VirtualPtr{
            static_cast<uintptr_t>(l0Index) << 39 |
            static_cast<uintptr_t>(l1Index) << 30 |
            static_cast<uintptr_t>(l2Index) << 21 |
            static_cast<uintptr_t>(l3Index) << 12
        };
        
        // We already tested the entry visit above, so we just need to test a single descriptor per view. We'll pick
        // the ones with non-zero type to ensure something is written to the table and it's not just zeroed out
        PageViewTest<::AArch64::PageTable::Level0View, ::AArch64::Descriptor::Table>("L0", virtualAddress, l0Index);
        PageViewTest<::AArch64::PageTable::Level1View, ::AArch64::Descriptor::Table>("L1", virtualAddress, l1Index);
        PageViewTest<::AArch64::PageTable::Level2View, ::AArch64::Descriptor::Table>("L2", virtualAddress, l2Index);
        PageViewTest<::AArch64::PageTable::Level3View, ::AArch64::Descriptor::Page>("L3", virtualAddress, l3Index);
    }
}