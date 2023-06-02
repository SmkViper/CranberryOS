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
                (BEToNative(static_cast<uint16_t>(aBigEndianNumber & 0x0000FFFF)) << 16);
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