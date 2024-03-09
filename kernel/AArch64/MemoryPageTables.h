#ifndef KERNEL_AARCH64_MEMORY_PAGE_TABLES_H
#define KERNEL_AARCH64_MEMORY_PAGE_TABLES_H

#include <cstdint>
#include <type_traits>

#include "../PointerTypes.h"
#include "MemoryDescriptor.h"

namespace UnitTests::AArch64::MemoryPageTables::Details
{
    struct TestAccessor;
}

namespace AArch64::PageTable
{
    // NOTE: We assume 4kb granule

    /////////////////////////////////////////////////
    // Virtual address layout:
    // +------+-----------+-----------+-----------+-----------+-------------+
    // |      | PGD Index | PUD Index | PMD Index | PTE Index | Page offset |
    // +------+-----------+-----------+-----------+-----------+-------------+
    // 63     47          38          29          20          11            0
    //
    // PGD Index - index into the Page Global Directory (level 0)
    // PUD Index - index into the Page Upper Directory (level 1)
    // PMD Index - index into the Page Middle Directory (level 2)
    // PTE Index - index into the Page Table Directory (level 3)
    // Page offset - offset of the physical address from the start of the page pointed at by the PTE entry
    //
    // For section mapping, the PTE Index is omitted, and bits 20:0 are used instead to offset into the 2mb section
    // pointed at by the PMD entry
    /////////////////////////////////////////////////
    constexpr uint8_t PageOffsetBits = 12;
    constexpr uint8_t TableIndexBits = 9;

    // the number of pointers in a single table is based on how many bits we have to index the table
    constexpr size_t PointersPerTable = 1ULL << TableIndexBits;

    namespace Details
    {
        struct VisitHelpers
        {
            /**
             * Helper for visit to see if aValue is of type TypeT and call aFunctor if it is
             * 
             * @param aFunctor The functor to call with TypeT if the type matches
             * @param aValue The value to check and pass to functor
             * 
             * @return True if the functor was called
             */
            template<typename TypeT, typename FunctorT>
            static bool TestAndVisit(FunctorT const& aFunctor, uint64_t const aValue)
            {
                auto const typeMatches = TypeT::IsType(aValue);
                if (typeMatches)
                {
                    aFunctor(TypeT{ aValue, Descriptor::Details::ValueConstructTag{} });
                }
                return typeMatches;
            }

            /**
             * Helper for visit to call aFunctor with a wrapped aValue based on what type it is
             * 
             * @param aFunctor Functor to call
             * @param aValue Value to pass in (as a wrapped type)
             */
            template<typename FunctorT, typename... DescriptorTypes>
            static void VisitImpl(FunctorT const& aFunctor, uint64_t const aValue)
            {
                (TestAndVisit<DescriptorTypes>(aFunctor, aValue) || ...);
            }
        };

        template<uint64_t AddressShift, class... DescriptorTypes>
        class PageView;

        /**
         * Tag to lock out certain constructors unless it's from an approved location
         */
        struct EntryConstructTag
        {
            // Let the page view and test code construct entries
            template<uint64_t AddressShift, class... DescriptorTypes>
            friend class PageView;
            friend struct UnitTests::AArch64::MemoryPageTables::Details::TestAccessor;
        private:
            // Cannot use =default because it may allow users to construct this without permission
            EntryConstructTag() {}; // NOLINT(hicpp-use-equals-default,modernize-use-equals-default)
        };

        /**
         * An entry in a table
         */
        template<typename... DescriptorTypes>
        class Entry
        {
        public:
            /**
             * Constructs an entry from the given raw value
             * 
             * @param aValue The value to make the entry from
             */
            Entry(uint64_t const aValue, EntryConstructTag /* aTag */)
                : Value{ aValue }
            {}

            /**
             * Calls functor with the descriptor type we're holding
             * 
             * @param aFunctor The functor to call, should have overloads of operator() for each descriptor type and
             * take them by value (modification's won't be sent back to this entry)
             */
            template<typename FunctorT>
            void Visit(FunctorT const& aFunctor) const
            {
                // #TODO: Would like to find a way to support modification of the entry directly, but we can work
                // around it for now. Maybe the Impl can make a local Entry on the stack, pass it in, then copy the
                // Value back
                VisitHelpers::VisitImpl<FunctorT, DescriptorTypes...>(aFunctor, Value);
            }

        private:
            uint64_t Value = 0;
        };

        /**
         * A "view" of the page table
         */
        template<uint64_t AddressShift, class... DescriptorTypes>
        class PageView
        {
            static constexpr uint64_t AddressMask = (PointersPerTable - 1);

            template<typename T>
            static constexpr bool ValidType = ((std::is_same_v<T, DescriptorTypes>) || ...);
        public:
            using Entry = Entry<DescriptorTypes...>;

            /**
             * Constructor, layering a view over the given memory assumed to be the table
             * 
             * @param apTable The table to make a view over - does not take ownership. Not expected to be null
             */
            explicit PageView(uint64_t* const apTable)
                : pTable{ apTable }
            {
                // #TODO: Should probably assert if apTable is null
            }

            /**
             * Gets the entry for the specified virtual address
             * 
             * @param aVirtualAddress The address to get the entry for
             * @return The entry in the table that contains that address
             */
            [[nodiscard]] Entry GetEntryForVA(VirtualPtr const aVirtualAddress) const
            {
                auto const tableIndex = (aVirtualAddress.GetAddress() >> AddressShift) & AddressMask;
                // #TODO: Assert if tableIndex is out of range
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                return Entry{ pTable[tableIndex], EntryConstructTag{} };
            }

            /**
             * Sets the entry for the specified virtual address
             * 
             * @param aVirtualAddress The address to set the entry for
             * @param aValue The value to set for that entry
             */
            template<typename DescriptorT, typename = std::enable_if_t<ValidType<DescriptorT>>>
            void SetEntryForVA(VirtualPtr const aVirtualAddress, DescriptorT const aValue) const
            {
                auto const tableIndex = (aVirtualAddress.GetAddress() >> AddressShift) & AddressMask;
                // #TODO: Assert if tableIndex is out of range
                DescriptorT::Write(aValue, pTable, tableIndex);
            }

            /**
             * Gets the table's pointer (what was given to the constructor)
             * 
             * @return The table's pointer
             */
            [[nodiscard]] uint64_t* GetTablePtr() const
            {
                return pTable;
            }

        private:
            uint64_t* pTable = nullptr;
        };
    }

    // Each entry covers 512GB of address space
    using Level0View = Details::PageView<PageOffsetBits + TableIndexBits * 3, Descriptor::Fault, Descriptor::Table>;
    // Each entry covers 1GB of address space
    using Level1View = Details::PageView<PageOffsetBits + TableIndexBits * 2, Descriptor::Fault, Descriptor::Table, Descriptor::L1Block>;
    // Each entry covers 2MB of address space
    using Level2View = Details::PageView<PageOffsetBits + TableIndexBits, Descriptor::Fault, Descriptor::Table, Descriptor::L2Block>;
    // Each entry covers 4KB of address space
    using Level3View = Details::PageView<PageOffsetBits, Descriptor::Fault, Descriptor::Page>;
}

#endif // KERNEL_AARCH64_MEMORY_PAGE_TABLES_H