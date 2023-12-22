#ifndef KERNEL_AARCH64_MEMORY_PAGE_TABLES_H
#define KERNEL_AARCH64_MEMORY_PAGE_TABLES_H

#include <cstdint>
#include <type_traits>

#include "MemoryDescriptor.h"

namespace AArch64
{
    namespace PageTable
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
        }

        /**
         * An entry in a table
        */
        template<typename... DescriptorTypes>
        class Entry
        {
            friend class Level0View;

            template<typename T>
            static constexpr bool ValidType = ((std::is_same_v<T, DescriptorTypes>) || ...);

        public:
            /**
             * Constructs an entry from a descriptor. Only valid if DescriptorT is one of the types the entry allows
             * 
             * @param aDescriptor The descriptor to construct from
            */
            template<typename DescriptorT, typename = std::enable_if_t<ValidType<DescriptorT>>>
            explicit Entry(DescriptorT const aDescriptor)
                : Value{ aDescriptor.GetValue() }
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
                // #TODO: Would be better if we matched std::visit and failed to compile if functor doesn't support all
                // descriptor types. And find a way to support modification of the entry directly. But we don't need
                // that yet
                Details::VisitHelpers::VisitImpl<FunctorT, DescriptorTypes...>(aFunctor, Value);
            }

        private:
            /**
             * Constructs an entry from the given value
             * 
             * @param aValue The value to make the entry from
            */
            explicit Entry(uint64_t const aValue)
                : Value{ aValue }
            {}

            uint64_t Value = 0;
        };

        /**
         * A "view" of the level 0 page table. Each entry covers 512GB of address space
        */
        class Level0View
        {
            static constexpr uint64_t AddressShift = PageOffsetBits + 3 * TableIndexBits;
            static constexpr uint64_t AddressMask = (PointersPerTable - 1);

            template<typename T>
            static constexpr bool ValidType = (std::is_same_v<T, Descriptor::Fault> || std::is_same_v<T, Descriptor::Table>);
        public:
            // Level 0 table only supports fault and table entries
            using Entry = PageTable::Entry<Descriptor::Fault, Descriptor::Table>;

            /**
             * Constructor, layering a view over the given memory assumed to be the level 0 table
             * 
             * @param apTable The table to make a view over - does not take ownership. Not expected to be null
            */
            explicit Level0View(uint64_t* const apTable)
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
            Entry GetEntryForVA(uintptr_t const aVirtualAddress) const
            {
                auto const tableIndex = (aVirtualAddress >> AddressShift) & AddressMask;
                // #TODO: Assert if tableIndex is out of range
                return Entry{ pTable[tableIndex] };
            }

            /**
             * Sets the entry for the specified virtual address
             * 
             * @param aVirtualAddress The address to set the entry for
             * @param aValue The value to set for that entry
            */
            template<typename DescriptorT, typename = std::enable_if_t<ValidType<DescriptorT>>>
            void SetEntryForVA(uintptr_t const aVirtualAddress, DescriptorT const aValue) const
            {
                auto const tableIndex = (aVirtualAddress >> AddressShift) & AddressMask;
                // #TODO: Assert if tableIndex is out of range
                DescriptorT::Write(aValue, pTable, tableIndex);
            }

        private:
            uint64_t* pTable = nullptr;
        };
    }
}

#endif // KERNEL_AARCH64_MEMORY_PAGE_TABLES_H