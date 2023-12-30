#include "MemoryDescriptor.h"

namespace AArch64
{
    namespace Descriptor
    {
        void Table::Write(Table const aValue, uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            apTable[aIndex] = aValue.DescriptorBits.to_ullong();
        }

        void Table::Address(PhysicalPtr const aAddress)
        {
            // #TODO: Should probably range check address to make sure the mask doesn't pull off any bits
            WriteMultiBitValue(DescriptorBits, aAddress.GetAddress(), Address_Mask, 0 /* no shift */);
        }

        PhysicalPtr Table::Address() const
        {
            return ReadMultiBitValue<PhysicalPtr>(DescriptorBits, Address_Mask, 0 /* no shift */);
        }

        void Page::Write(Page const aValue, uint64_t apTable[], size_t const aIndex)
        {
            // #TODO: Range-check index with pointers per table - or make an array view type
            apTable[aIndex] = aValue.DescriptorBits.to_ullong();
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

        void Page::Address(PhysicalPtr const aAddress)
        {
            // #TODO: Should probably range check address to make sure the mask doesn't pull off any bits
            WriteMultiBitValue(DescriptorBits, aAddress.GetAddress(), Address_Mask, 0 /* no shift */);
        }

        PhysicalPtr Page::Address() const
        {
            return ReadMultiBitValue<PhysicalPtr>(DescriptorBits, Address_Mask, 0 /* no shift */);
        }
    }
}