#include "MemoryDescriptor.h"

#include "../Utils.h"

namespace AArch64
{
    namespace Descriptor
    {
        void Table::Write(Table const aValue, uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            apTable[aIndex] = aValue.DescriptorBits.to_ullong();
        }

        Table Table::Read(uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            // #TODO: Check the entry type to make sure it's a table entry
            return Table{ apTable[aIndex] };
        }

        void Table::Address(uintptr_t const aAddress)
        {
            // #TODO: Should probably range check address to make sure the mask doesn't pull off any bits
            WriteMultiBitValue(DescriptorBits, aAddress, Address_Mask, 0 /* no shift */);
        }

        uintptr_t Table::Address() const
        {
            return ReadMultiBitValue<uintptr_t>(DescriptorBits, Address_Mask, 0 /* no shift */);
        }

        void Block::Write(Block const aValue, uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            apTable[aIndex] = aValue.DescriptorBits.to_ullong();
        }

        Block Block::Read(uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            // #TODO: Check the entry type to make sure it's a block entry
            return Block{ apTable[aIndex] };
        }

        void Block::AttrIndx(uint8_t const aIndex)
        {
            // #TODO: Range check aIndex
            WriteMultiBitValue(DescriptorBits, aIndex, AttrIndxIndex_Mask, AttrIndxIndex_Shift);
        }

        uint8_t Block::AttrIndx() const
        {
            return ReadMultiBitValue<uint8_t>(DescriptorBits, AttrIndxIndex_Mask, AttrIndxIndex_Shift);
        }

        void Block::AP(AccessPermissions const aPermission)
        {
            WriteMultiBitValue(DescriptorBits, aPermission, APIndex_Mask, APIndex_Shift);
        }

        Block::AccessPermissions Block::AP() const
        {
            return ReadMultiBitValue<AccessPermissions>(DescriptorBits, APIndex_Mask, APIndex_Shift);
        }

        void Block::L1Address(uintptr_t const aAddress)
        {
            // #TODO: Should probably range check address to make sure the mask doesn't pull off any bits
            WriteMultiBitValue(DescriptorBits, aAddress, L1Address_Mask, 0 /* no shift */);
        }

        uintptr_t Block::L1Address() const
        {
            return ReadMultiBitValue<uintptr_t>(DescriptorBits, L1Address_Mask, 0 /* no shift */);
        }

        void Block::L2Address(uintptr_t const aAddress)
        {
            // #TODO: Should probably range check address to make sure the mask doesn't pull off any bits
            WriteMultiBitValue(DescriptorBits, aAddress, L2Address_Mask, 0 /* no shift */);
        }

        uintptr_t Block::L2Address() const
        {
            return ReadMultiBitValue<uintptr_t>(DescriptorBits, L2Address_Mask, 0 /* no shift */);
        }

        void Page::Write(Page const aValue, uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            apTable[aIndex] = aValue.DescriptorBits.to_ullong();
        }

        Page Page::Read(uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            // #TODO: Check the entry type to make sure it's a table entry
            return Page{ apTable[aIndex] };
        }

        void Page::AttrIndx(uint8_t const aIndex)
        {
            // #TODO: Range check aIndex
            WriteMultiBitValue(DescriptorBits, aIndex, AttrIndxIndex_Mask, AttrIndxIndex_Shift);
        }

        uint8_t Page::AttrIndx() const
        {
            return ReadMultiBitValue<uint8_t>(DescriptorBits, AttrIndxIndex_Mask, AttrIndxIndex_Shift);
        }

        void Page::AP(AccessPermissions const aPermission)
        {
            WriteMultiBitValue(DescriptorBits, aPermission, APIndex_Mask, APIndex_Shift);
        }

        Page::AccessPermissions Page::AP() const
        {
            return ReadMultiBitValue<AccessPermissions>(DescriptorBits, APIndex_Mask, APIndex_Shift);
        }

        void Page::Address(uintptr_t const aAddress)
        {
            // #TODO: Should probably range check address to make sure the mask doesn't pull off any bits
            WriteMultiBitValue(DescriptorBits, aAddress, Address_Mask, 0 /* no shift */);
        }

        uintptr_t Page::Address() const
        {
            return ReadMultiBitValue<uintptr_t>(DescriptorBits, Address_Mask, 0 /* no shift */);
        }
    }
}