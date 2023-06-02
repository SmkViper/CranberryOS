#include <cstring>

#include "DeviceTree.h"
#include "../Print.h"
#include "../MiniUart.h"

namespace DeviceTree
{
    namespace
    {
        constexpr uint32_t ExpectedMagic = 0xd00dfeed;
        constexpr uint32_t ExpectedVersion = 17;

        // #TODO: We can probably be a bit smarter about this and encode the endianness in the type in some manner

        // From DeviceTree specification, section 5.2
        struct fdt_header
        {
            // NOTE: All values are big-endian when loaded from memory
            uint32_t magic = 0; // "Magic" number to verify the header is valid
            uint32_t totalsize = 0; // The total size of the device tree blob, including all padding
            uint32_t off_dt_struct = 0; // Offset to the structure block from the header, in bytes
            uint32_t off_dt_strings = 0; // Offset to the strings block from the header, in bytes
            uint32_t off_mem_rsvmap = 0; // Offset to the memory reservation block from the header, in bytes
            uint32_t version = 0; // The version of the data structure
            uint32_t last_comp_version = 0; // The lowest version which this structure is backwards compatible with
            uint32_t boot_cpuid_phys = 0; // Physical ID of the boot CPU. Same as the "reg" property of the CPU node
            uint32_t size_dt_strings = 0; // Length in bytes of the strings block
            uint32_t size_dt_struct = 0; // Length in bytes of the structs block
        };

        // From DeviceTree specification, section 5.3.2
        struct fdt_reserve_entry
        {
            uint64_t address = 0; // start of the reserved block
            uint64_t size = 0; // size of the reserved block
        };

        // From DeviceTree specification, section 5.4.1
        // Token followed by zero-terminated string containing the name and unit address. 0 padded to 4 bytes
        constexpr uint32_t FDT_BEGIN_NODE = 0x01;
        // Token has no extra data
        constexpr uint32_t FDT_END_NODE = 0x02;
        // Token followed by fdt_prop_extra_data, then zero-terminated value. 0 padded to 4 bytes
        constexpr uint32_t FDT_PROP = 0x03;
        // Token has no extra data
        constexpr uint32_t FDT_NOP = 0x04;
        // Token has no extra data. Byte address following should be off_det_struct + size_dt_struct
        constexpr uint32_t FDT_END = 0x09;

        struct fdt_prop_extra_data
        {
            uint32_t len = 0; // length of the property's value in bytes (may be 0)
            uint32_t nameoff = 0; // offset into the strings block where the name is stored
        };

        // #TODO: A lot of these utilities should probably be moved somewhere for conveinience

        /**
         * Converts a big-endian number to native endian (for our kernel)
         * 
         * @param aBigEndianNumber The number to convert
         * @return The converted number
         */
        constexpr uint16_t BEToNative(uint16_t const aBigEndianNumber)
        {
            return ((aBigEndianNumber & 0x00FF) << 8) | ((aBigEndianNumber & 0xFF00) >> 8);
        }

        /**
         * Converts a big-endian number to native endian (for our kernel)
         * 
         * @param aBigEndianNumber The number to convert
         * @return The converted number
         */
        constexpr uint32_t BEToNative(uint32_t const aBigEndianNumber)
        {
            return BEToNative(static_cast<uint16_t>((aBigEndianNumber & 0xFFFF0000) >> 16)) |
                (static_cast<uint32_t>(BEToNative(static_cast<uint16_t>(aBigEndianNumber & 0x0000FFFF))) << 16);
        }

        /**
         * Converts a big-endian number to native endian (for our kernel)
         * 
         * @param aBigEndianNumber The number to convert
         * @return The converted number
         */
        constexpr uint64_t BEToNative(uint64_t const aBigEndianNumber)
        {
            return BEToNative(static_cast<uint32_t>((aBigEndianNumber & 0xFFFF'FFFF'0000'0000) >> 32)) |
                (static_cast<uint64_t>(BEToNative(static_cast<uint32_t>(aBigEndianNumber & 0x0000'0000'FFFF'FFFF))) << 32);
        }

        /**
         * Converts a big-endian header to native endian (for our kernel)
         * 
         * @param aBigEndianStruct The header to convert
         * @return The converted header
         */
        constexpr fdt_header BEToNative(fdt_header const& aBigEndianStruct)
        {
            fdt_header result;
            result.magic = BEToNative(aBigEndianStruct.magic);
            result.totalsize = BEToNative(aBigEndianStruct.totalsize);
            result.off_dt_struct = BEToNative(aBigEndianStruct.off_dt_struct);
            result.off_dt_strings = BEToNative(aBigEndianStruct.off_dt_strings);
            result.off_mem_rsvmap = BEToNative(aBigEndianStruct.off_mem_rsvmap);
            result.version = BEToNative(aBigEndianStruct.version);
            result.last_comp_version = BEToNative(aBigEndianStruct.last_comp_version);
            result.boot_cpuid_phys = BEToNative(aBigEndianStruct.boot_cpuid_phys);
            result.size_dt_strings = BEToNative(aBigEndianStruct.size_dt_strings);
            result.size_dt_struct = BEToNative(aBigEndianStruct.size_dt_struct);
            return result;
        }

        /**
         * Converts a big-endian entry to native endian (for our kernel)
         * 
         * @param aBigEndianStruct The entry to convert
         * @return The converted header
         */
        constexpr fdt_reserve_entry BEToNative(fdt_reserve_entry const& aBigEndianStruct)
        {
            fdt_reserve_entry result;
            result.address = BEToNative(aBigEndianStruct.address);
            result.size = BEToNative(aBigEndianStruct.size);
            return result;
        }

        /**
         * Converts a big-endian extra data to native endian (for our kernel)
         * 
         * @param aBigEndianStruct The extra data to convert
         * @return The converted header
         */
        constexpr fdt_prop_extra_data BEToNative(fdt_prop_extra_data const& aBigEndianStruct)
        {
            fdt_prop_extra_data result;
            result.len = BEToNative(aBigEndianStruct.len);
            result.nameoff = BEToNative(aBigEndianStruct.nameoff);
            return result;
        }

        /**
         * Aligns the given pointer
         * 
         * @param apPtr The pointer to alignt
         * @param aAlignment The bytes to align it to (i.e. 4 for 32-bit integers)
         * @return The aligned pointer
         */
        uint8_t const* AlignPointer(uint8_t const* apPtr, std::size_t aAlignment)
        {
            auto const padding = (aAlignment - (reinterpret_cast<uintptr_t>(apPtr) % aAlignment)) % aAlignment;
            return apPtr + padding;
        }

        /**
         * Outputs the header to UART
         * 
         * @param aHeader The header to output
         */
        void OutputHeader(fdt_header const& aHeader)
        {
            MiniUART::SendString("fdt_header:\r\n");
            Print::FormatToMiniUART("\tMagic: {:x}\r\n", aHeader.magic);
            Print::FormatToMiniUART("\tTotal size: {} bytes\r\n", aHeader.totalsize);
            Print::FormatToMiniUART("\tStruct table offset (size): {:x} ({} bytes)\r\n", aHeader.off_dt_struct, aHeader.size_dt_struct);
            Print::FormatToMiniUART("\tString table offset (size): {:x} ({} bytes)\r\n", aHeader.off_dt_strings, aHeader.size_dt_strings);
            Print::FormatToMiniUART("\tMemory reservation map offset: {:x}\r\n", aHeader.off_mem_rsvmap);
            Print::FormatToMiniUART("\tVersion (comp version): {} ({})\r\n", aHeader.version, aHeader.last_comp_version);
            Print::FormatToMiniUART("\tBoot CPU ID: {:x}\r\n", aHeader.boot_cpuid_phys);
        }

        /**
         * Output the memory reservation map to the UART
         * 
         * @param aHeader The header containing the map offset
         * @param aBaseAddr The base address for the offset
         */
        void OutputMemoryReservationMap(fdt_header const& aHeader, uint8_t const* const aBaseAddr)
        {
            MiniUART::SendString("Memory reservation map:\r\n");
            uint8_t const* pcurEntry = aBaseAddr + aHeader.off_mem_rsvmap;
            auto done = false;
            while (!done)
            {
                fdt_reserve_entry entry;
                std::memcpy(&entry, pcurEntry, sizeof(entry));

                entry = BEToNative(entry);

                done = (entry.address == 0) && (entry.size == 0);
                if (!done)
                {
                    Print::FormatToMiniUART("\tAddress (size): {:x} ({} bytes)\r\n", entry.address, entry.size);
                    pcurEntry += sizeof(entry);
                }
            }
        }

        /**
         * Outputs a begin node with its extra data
         * 
         * @param apExtraData The location of the extra data in the table
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputBeginNode(uint8_t const* const apExtraData)
        {
            char const* pnodeName = reinterpret_cast<char const*>(apExtraData);
            Print::FormatToMiniUART("\tFDT_BEGIN_NODE: \"{}\"\r\n", pnodeName);
            auto const nameByteLen = std::strlen(pnodeName) + 1; // including terminator
            return AlignPointer(apExtraData + nameByteLen, alignof(uint32_t));
        }

        /**
         * Outputs an end node with its extra data
         * 
         * @param apExtraData The location of the extra data in the table
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputEndNode(uint8_t const* const apExtraData)
        {
            // No extra data
            MiniUART::SendString("\tFDT_END_NODE\r\n");
            return apExtraData;
        }

        /**
         * Outputs a property with its extra data
         * 
         * @param aHeader Header containing offsets and other information
         * @param aBaseAddr Base address for offsets in the header
         * @param apExtraData The location of the extra data in the table
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputProp(fdt_header const& aHeader, uint8_t const* const aBaseAddr, uint8_t const* const apExtraData)
        {
            fdt_prop_extra_data dataHeader;
            std::memcpy(&dataHeader, apExtraData, sizeof(dataHeader));
            uint8_t const* pendPtr = apExtraData + sizeof(dataHeader);

            dataHeader = BEToNative(dataHeader);

            char const* pname = reinterpret_cast<char const*>(aBaseAddr + aHeader.off_dt_strings + dataHeader.nameoff);
            Print::FormatToMiniUART("\t\tFDT_PROP:\r\n\t\t\tName: {}\r\n\t\t\tValue:", pname);
            
            if (dataHeader.len == 0)
            {
                MiniUART::SendString(" <empty>\r\n");
            }
            else
            {
                for (auto curByte = 0; curByte != dataHeader.len; ++curByte, ++pendPtr)
                {
                    Print::FormatToMiniUART(" {:x}", *pendPtr);
                }
                MiniUART::SendString("\r\n");
            }
            return AlignPointer(pendPtr, 4);
        }

        /**
         * Outputs a nop with its extra data
         * 
         * @param apExtraData The location of the extra data in the table
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputNop(uint8_t const* const apExtraData)
        {
            // No extra data
            MiniUART::SendString("\tFDT_NOP\r\n");
            return apExtraData;
        }

        /**
         * Outputs an end with its extra data
         * 
         * @param apExtraData The location of the extra data in the table
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputEnd(uint8_t const* const apExtraData)
        {
            // No extra data
            MiniUART::SendString("\tFDT_END\r\n");
            return apExtraData;
        }

        /**
         * Outputs the list of tokens to the UART
         * 
         * @param aHeader The header containing the block offset and other data needed
         * @param aBaseAddr The base address for the offsets
         */
        void OutputTokenList(fdt_header const& aHeader, uint8_t const* const aBaseAddr)
        {
            MiniUART::SendString("Structure block:\r\n");
            uint8_t const* pcurToken = aBaseAddr + aHeader.off_dt_struct;
            auto done = false;
            while (!done)
            {
                uint32_t token = 0;
                std::memcpy(&token, pcurToken, sizeof(token));
                pcurToken += sizeof(token);

                token = BEToNative(token);

                switch (token)
                {
                case FDT_BEGIN_NODE:
                    pcurToken = OutputBeginNode(pcurToken);
                    break;

                case FDT_END_NODE:
                    pcurToken = OutputEndNode(pcurToken);
                    break;

                case FDT_PROP:
                    pcurToken = OutputProp(aHeader, aBaseAddr, pcurToken);
                    break;

                case FDT_NOP:
                    pcurToken = OutputNop(pcurToken);
                    break;

                case FDT_END:
                    pcurToken = OutputEnd(pcurToken);
                    done = true;
                    break;

                default:
                    Print::FormatToMiniUART("Unknown token {}, aborting\r\n", token);
                    done = true;
                    break;
                }

                if ((pcurToken >= (aBaseAddr + aHeader.off_dt_struct + aHeader.size_dt_struct)) && !done)
                {
                    MiniUART::SendString("Ran off the end of the table, aborting\r\n");
                    done = true;
                }
            }
        }
    }

    /**
     * Parse a device tree binary blob
     * 
     * @param apDTB The device tree blob to read
     */
    void ParseDeviceTree(uint8_t const* const apDTB)
    {
        Print::FormatToMiniUART("Loading DTB from: {:x}...\r\n", reinterpret_cast<uintptr_t>(apDTB));

        fdt_header header;
        std::memcpy(&header, apDTB, sizeof(header));

        header = BEToNative(header);

        if (header.magic == ExpectedMagic)
        {
            MiniUART::SendString("Magic matches!\r\n");
            if ((header.version >= ExpectedVersion) && (header.last_comp_version <= ExpectedVersion))
            {
                MiniUART::SendString("Version check passes!\r\n");

                OutputHeader(header);
                OutputMemoryReservationMap(header, apDTB);
                OutputTokenList(header, apDTB);
                // #TODO: Handle more of the device tree
            }
            else
            {
                Print::FormatToMiniUART("Version check FAILED. Version: {} (last compatible version: {}). Expected version: {}",
                    header.version, header.last_comp_version, ExpectedVersion);
            }
        }
        else
        {
            Print::FormatToMiniUART("Magic mismatch, found {:x}, expected {:x}\r\n", header.magic, ExpectedMagic);
        }
    }
}