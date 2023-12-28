#include "MemoryPageTablesTests.h"

#include <cstring>
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
                    // Hack to get around the descriptor bits being private
                    static_assert(sizeof(tableBits) == sizeof(aDescriptor), "Unexpected size difference");
                    memcpy(&tableBits, &aDescriptor, sizeof(tableBits));
                }
            });

            EmitTestResult(faultVisitFault && !faultVisitTable, "Entry Visit for fault");
            EmitTestResult(!tableVisitFault && tableVisitTable
                && (tableBits == 0b1111)
                , "Entry Visit for table");
        }

        // #TODO: PageView tests
    }

    void Run()
    {
        EntryTest();
    }
}