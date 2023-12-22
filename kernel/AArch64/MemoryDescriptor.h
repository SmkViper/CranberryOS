#ifndef KERNEL_AARCH64_MEMORY_DESCRIPTOR_H
#define KERNEL_AARCH64_MEMORY_DESCRIPTOR_H

#include <cstdint>
#include <bitset>

namespace AArch64
{
    namespace PageTable
    {
        namespace Details
        {
            struct VisitHelpers;
        }
    }

    namespace Descriptor
    {
        // Table descriptor format: https://developer.arm.com/documentation/ddi0487/
        // Section D8.3

        namespace Details
        {
            constexpr uint64_t TypeMask = 0b11; // lowest two bits is the type

            /**
             * Tag to lock out certain constructors unless it's from an approved location
            */
            struct ValueConstructTag
            {
                // Let the entry visitors make descriptors from raw values
                friend struct PageTable::Details::VisitHelpers;
            private:
                ValueConstructTag() {};
            };
        }

        /**
         * A fault descriptor (any descriptor where the lowest bit is 0). Causes a MMU fault if accessed
        */
        class Fault
        {
            static constexpr uint64_t Type = 0b00;
        public:
            /**
             * Constructor, sets all bits to zero
            */
            Fault() = default;

            /**
             * Constructor from a specific value
             * 
             * @param aValue The value to construct from
            */
            Fault(uint64_t /*aValue*/, Details::ValueConstructTag) {}

            /**
             * Checks to see if the value represents a descriptor of this type
             * 
             * @param aValue The value to check
             * 
             * @return True if the value is a descriptor of this type
            */
            static bool IsType(uint64_t const aValue)
            {
                return (aValue & Details::TypeMask) == Type;
            }

        private:
            // We don't actually use these bits directly (and a faulting descriptor is represented by any value with
            // the low bit zeroed, but this gives us an easy one)
            uint64_t DescriptorBits [[maybe_unused]] = 0;
        };

        /**
         * An entry in table 0, 1, or 2 that points at another table
        */
        class Table
        {
            static constexpr uint64_t Type = 0b11;
        public:
            /**
             * Constructor, sets type bits, but everything else is zeroed
            */
            Table()
                : Table{ Type }
            {}

            /**
             * Constructor from a specific value
             * 
             * @param aValue The value to construct from
            */
            Table(uint64_t aValue, Details::ValueConstructTag) : Table{ aValue } {}

            /**
             * Writes the given entry to the table
             * 
             * @param aValue Value to write
             * @param apTable Table to write to
             * @param aIndex Index to write to in the table
            */
            static void Write(Table aValue, uint64_t apTable[], size_t aIndex);

            /**
             * Reads the given entry from the table (assumes entry is a table entry)
             * 
             * @param apTable Table to read from
             * @param aIndex Index to read from in the table
             * @return The read table entry
            */
            static Table Read(uint64_t apTable[], size_t aIndex);

            /**
             * Checks to see if the value represents a descriptor of this type
             * 
             * @param aValue The value to check
             * 
             * @return True if the value is a descriptor of this type
            */
            static bool IsType(uint64_t const aValue)
            {
                return (aValue & Details::TypeMask) == Type;
            }

            /**
             * Sets the table address this entry points at
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @param aAddress The table address
            */
            void Address(uintptr_t aAddress);

            /**
             * Obtains the table address this entry points at
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @return The table address
            */
            uintptr_t Address() const;

        private:
            /**
             * Create a descriptor from the given bits
             * 
             * @param aInitialValue The bits to start with
            */
            explicit Table(uint64_t const aInitialValue)
                : DescriptorBits{ aInitialValue }
            {}

            // We assume 4kb granule and 48 bits of address
            //
            // Type             [1:0]   (0b11)
            // Ignored          [11:2]
            static constexpr uint64_t Address_Mask = 0x0000'FFFF'FFFF'F000; // bits [47:12]
            // Reserved         [50:48] (Res0)
            // Ignored          [58:51]
            // PXNTable         [59]
            // UXNTable/XNTable [60]
            // APTable          [62:61]
            // NSTable          [63]

            std::bitset<64> DescriptorBits;
        };

        /**
         * An entry in table 1 or 2 that points at a block of memory (larger than a page)
        */
        class Block
        {
        public:
            /**
             * Constructor, sets type bits, but everything else is zeroed
            */
            Block()
                : Block{ 0b01 }
            {}

            /**
             * Writes the given entry to the table
             * 
             * @param aValue Value to write
             * @param apTable Table to write to
             * @param aIndex Index to write to in the table
            */
            static void Write(Block aValue, uint64_t apTable[], size_t aIndex);

            /**
             * Reads the given entry from the table (assumes entry is a block entry)
             * 
             * @param apTable Table to read from
             * @param aIndex Index to read from in the table
             * @return The read block entry
            */
            static Block Read(uint64_t apTable[], size_t aIndex);

            /**
             * Sets index of the attributes for this block in the MIAR_ELx register
             * 
             * @param aIndex The attributes index
            */
            void AttrIndx(uint8_t aIndex);

            /**
             * Gets the index of the attributes for this block in the MIAR_ELx register
             * 
             * @return The attributes index
            */
            uint8_t AttrIndx() const;

            enum class AccessPermissions: uint8_t
            {
                KernelRWUserNone =  0b00,
                KernelRWUserRW =    0b01,
                KernelROUserNone =  0b10,
                KernelROUserRO =    0b11,
            };

            /**
             * Sets the access permission for this block
             * 
             * @param aPermission The block permission
            */
            void AP(AccessPermissions aPermission);

            /**
             * Obtains the access permission for this block
             * 
             * @return The block permission
            */
            AccessPermissions AP() const;

            /**
             * AF Bit - Access flag
             * 
             * @param aAccess Sets the access flag - false flags won't be cached and generate an access flag fault if
             * hardware doesn't manage the flag (FEAT_HAFDBS)
            */
            void AF(bool const aAccess) { DescriptorBits[AFIndex] = aAccess; }

            /**
             * AF Bit - Access flag
             * 
             * @return True if the memory has been accessed since it was last set to false
            */
            bool AF() const { return DescriptorBits[AFIndex]; }

            /**
             * Sets the block address this entry points at (assuming this entry is in a level 1 table)
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @param aAddress The block address (1GB block)
            */
            void L1Address(uintptr_t aAddress);

            /**
             * Obtains the block address this entry points at (assuming this entry is in a level 1 table)
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @return The block address (1GB block)
            */
            uintptr_t L1Address() const;

            /**
             * Sets the block address this entry points at (assuming this entry is in a level 2 table)
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @param aAddress The block address (2MB block)
            */
            void L2Address(uintptr_t aAddress);

            /**
             * Obtains the block address this entry points at (assuming this entry is in a level 2 table)
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @return The block address (2MB block)
            */
            uintptr_t L2Address() const;

        private:
            /**
             * Create a descriptor from the given bits
             * 
             * @param aInitialValue The bits to start with
            */
            explicit Block(uint64_t const aInitialValue)
                : DescriptorBits{ aInitialValue }
            {}

            // #TODO: Could probably improve type safety around L1/L2 block descriptors
            
            // We assume 4kb granule and 48 bits of address
            //
            // Type         [1:0]   (0b01)
            static constexpr unsigned AttrIndxIndex_Shift = 2; // bits [4:2]
            static constexpr uint64_t AttrIndxIndex_Mask = 0b111;
            // NS           [5]
            static constexpr unsigned APIndex_Shift = 6; // bits [7:6]
            static constexpr uint64_t APIndex_Mask = 0b11;
            // SH[1:0]      [9:8]
            static constexpr unsigned AFIndex = 10;
            // NSE/nG       [11]
            // Reserved     [15:12] (Res0)
            // nT           [16]    (Res0 if FEAT_BBM is not implemented)
            // Reserved     [n:17]  (Res0) L1 n = 29, L2 n = 20
            static constexpr uint64_t L1Address_Mask = 0x0000'FFFF'C000'0000; // bits [47:30]
            static constexpr uint64_t L2Address_Mask = 0x0000'FFFF'FFE0'0000; // bits [47:21]
            // Reserved     [49:48] (Res0)
            // GP           [50]    (Res0 if FEAT_BTI not implemented)
            // DBM          [51]    (Res0 if FEAT_HAFDBS not implemented)
            // Contiguous   [52]
            // PXN          [53]
            // UXN/XN       [54]
            // Ignored      [58:55] (Reserved for software use)
            // PBHA         [62:59] (Ignored if FEAT_HPDS2 not implemented)
            // Ignored      [63]

            std::bitset<64> DescriptorBits;
        };

        /**
         * An entry in table 3 that points at a page
        */
        class Page
        {
        public:
            /**
             * Constructor, sets type bits, but everything else is zeroed
            */
            Page()
                : Page{ 0b11 }
            {}

            /**
             * Writes the given entry to the table
             * 
             * @param aValue Value to write
             * @param apTable Table to write to
             * @param aIndex Index to write to in the table
            */
            static void Write(Page aValue, uint64_t apTable[], size_t aIndex);

            /**
             * Reads the given entry from the table (assumes entry is a page entry)
             * 
             * @param apTable Table to read from
             * @param aIndex Index to read from in the table
             * @return The read page entry
            */
            static Page Read(uint64_t apTable[], size_t aIndex);

            /**
             * Sets index of the attributes for this page in the MIAR_ELx register
             * 
             * @param aIndex The attributes index
            */
            void AttrIndx(uint8_t aIndex);

            /**
             * Gets the index of the attributes for this page in the MIAR_ELx register
             * 
             * @return The attributes index
            */
            uint8_t AttrIndx() const;

            enum class AccessPermissions: uint8_t
            {
                KernelRWUserNone =  0b00,
                KernelRWUserRW =    0b01,
                KernelROUserNone =  0b10,
                KernelROUserRO =    0b11,
            };

            /**
             * Sets the access permission for this page
             * 
             * @param aPermission The page permission
            */
            void AP(AccessPermissions aPermission);

            /**
             * Obtains the access permission for this page
             * 
             * @return The page permission
            */
            AccessPermissions AP() const;

            /**
             * AF Bit - Access flag
             * 
             * @param aAccess Sets the access flag - false flags won't be cached and generate an access flag fault if
             * hardware doesn't manage the flag (FEAT_HAFDBS)
            */
            void AF(bool const aAccess) { DescriptorBits[AFIndex] = aAccess; }

            /**
             * AF Bit - Access flag
             * 
             * @return True if the memory has been accessed since it was last set to false
            */
            bool AF() const { return DescriptorBits[AFIndex]; }

            /**
             * Sets the block address this entry points at
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @param aAddress The block address
            */
            void Address(uintptr_t aAddress);

            /**
             * Obtains the block address this entry points at
             * #TODO: Pretty sure this is a physical address, so need a type for that
             * 
             * @return The block address
            */
            uintptr_t Address() const;

        private:
            /**
             * Create a descriptor from the given bits
             * 
             * @param aInitialValue The bits to start with
            */
            explicit Page(uint64_t const aInitialValue)
                : DescriptorBits{ aInitialValue }
            {}

            // We assume 4kb granule and 48 bits of address
            //
            // Type         [1:0]   (0b11)
            static constexpr unsigned AttrIndxIndex_Shift = 2; // bits [4:2]
            static constexpr uint64_t AttrIndxIndex_Mask = 0b111;
            // NS           [5]
            static constexpr unsigned APIndex_Shift = 6; // bits [7:6]
            static constexpr uint64_t APIndex_Mask = 0b11;
            // SH[1:0]      [9:8]
            static constexpr unsigned AFIndex = 10;
            // NSE/nG       [11]
            static constexpr uint64_t Address_Mask = 0x0000'FFFF'FFFF'F000; // bits [47:12]
            // Reserved     [49:48] (Res0)
            // GP           [50]    (Res0 if FEAT_BTI not implemented)
            // DBM          [51]    (Res0 if FEAT_HAFDBS not implemented)
            // Contiguous   [52]
            // PXN          [53]
            // UXN/XN       [54]
            // Ignored      [58:55] (Reserved for software use)
            // PBHA         [62:59] (Ignored if FEAT_HPDS2 not implemented)
            // Ignored      [63]

            std::bitset<64> DescriptorBits;
        };
        
        // #TODO: We're going to want a read function that can read a descriptor and return the right type
    }
}

#endif // KERNEL_AARCH64_MEMORY_DESCRIPTOR_H