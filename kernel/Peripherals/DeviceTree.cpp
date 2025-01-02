#include "DeviceTree.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include "../BigEndian.h"
#include "../PointerTypes.h"
#include "../Print.h"
#include "../MiniUart.h"

namespace DeviceTree
{
    namespace
    {
        constexpr uint32_t ExpectedMagic = 0xd00dfeed;
        constexpr uint32_t ExpectedVersion = 17;

        // #TODO: There are a lot of NOLINT comments due to all the casting and pointer math being done to read the raw
        // memory. Would be nice if we could figure out a way to handle it better

        // From DeviceTree specification, section 5.3.2
        struct fdt_reserve_entry
        {
            BigEndian<uint64_t> address = 0; // start of the reserved block
            BigEndian<uint64_t> size = 0; // size of the reserved block
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
            BigEndian<uint32_t> len = 0; // length of the property's value in bytes (may be 0)
            BigEndian<uint32_t> nameoff = 0; // offset into the strings block where the name is stored
        };

        // Holds information that some properties need to extract their data. Comes from #address-cells and #size-cells
        // properties on the given node. See DeviceTree specification section 2.3.5
        struct CellInformation
        {
            uint32_t AddressCells = 2; // Number of 32-bit cells addresses are composed of
            uint32_t SizeCells = 1; // Number of 32-bit cells sizes are composed of
        };

        // We can't allocate memory at this point, so this is a really dumb stack to keep track of cell information
        // #TODO: Can likely come up with a better system, like code that can find nodes relative to other nodes or by
        // path/name. Alternatively, a static stack like this might be useful elsewhere
        class CellInformationStack
        {
        public:
            CellInformationStack() = default;
            CellInformationStack(CellInformationStack const&) = delete;
            CellInformationStack(CellInformationStack&&) = delete;
            ~CellInformationStack() = default;
            CellInformationStack const& operator=(CellInformationStack const&) = delete;
            CellInformationStack const& operator=(CellInformationStack&&) = delete;

            /**
             * Pushes a new bit of cell information with default values onto the stack
             * 
             * @return True if there was space and a new value was pushed
             */
            [[nodiscard]] bool Push();

            /**
             * Pops the top bit of cell information off the stack
             * 
             * @return True if there was something popped
             */
            [[nodiscard]] bool Pop();

            /**
             * Obtains the top of the stack, or defaults if the stack is empty
             * 
             * @return The top of the stack, or default values
             */
            [[nodiscard]] CellInformation Top() const;

            /**
             * Sets the values on the current top, or does nothing if empty
             * 
             * @param aNewTop The new top values
             * @return True if values were set, false if they were ignored
             */
            bool SetTop(CellInformation aNewTop);

            /**
             * Checks to see if the stack is empty
             * 
             * @return True if stack is empty
             */
            [[nodiscard]] bool Empty() const { return Size == 0; }
        private:
            static constexpr uint32_t MaxSizeCS = 10;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            CellInformation Stack[MaxSizeCS] = {};
            uint32_t Size = 0;
        };

        bool CellInformationStack::Push()
        {
            if (Size >= MaxSizeCS)
            {
                return false;
            }
            Stack[Size] = CellInformation{}; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            ++Size;
            return true;
        }

        bool CellInformationStack::Pop()
        {
            if (Size == 0)
            {
                return false;
            }
            --Size;
            return true;
        }

        CellInformation CellInformationStack::Top() const
        {
            if (Size == 0)
            {
                return CellInformation{};
            }
            return Stack[Size - 1U]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        bool CellInformationStack::SetTop(CellInformation const aNewTop)
        {
            if (Size == 0)
            {
                return false;
            }
            Stack[Size - 1U] = aNewTop; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            return true;
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
            auto const padding = (aAlignment - (std::bit_cast<uintptr_t>(apPtr) % aAlignment)) % aAlignment;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            uint8_t const* pcurEntry = aBaseAddr + aHeader.off_mem_rsvmap;
            auto done = false;
            while (!done)
            {
                fdt_reserve_entry entry;
                std::memcpy(&entry, pcurEntry, sizeof(entry));

                done = (entry.address == 0) && (entry.size == 0);
                if (!done)
                {
                    Print::FormatToMiniUART("\tAddress (size): {:x} ({} bytes)\r\n", entry.address, entry.size);
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    pcurEntry += sizeof(entry);
                }
            }
        }

        /**
         * Helper to indent UART output by the specified number of levels
         * 
         * @param aIndentLevel Number of levels to indent
         */
        void IndentOutput(uint32_t const aIndentLevel)
        {
            for (auto curIndent = 0U; curIndent < aIndentLevel; ++curIndent)
            {
                MiniUART::SendString("  ");
            }
        }

        /**
         * Outputs a begin node with its extra data
         * 
         * @param apExtraData The location of the extra data in the table
         * @param aIndentLevel Level of indentation to use
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputBeginNode(uint8_t const* const apExtraData, uint32_t const aIndentLevel)
        {
            IndentOutput(aIndentLevel);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            char const* pnodeName = reinterpret_cast<char const*>(apExtraData);
            Print::FormatToMiniUART("{} {{\r\n", pnodeName);
            auto const nameByteLen = std::strlen(pnodeName) + 1; // including terminator
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return AlignPointer(apExtraData + nameByteLen, alignof(uint32_t));
        }

        /**
         * Outputs an end node with its extra data
         * 
         * @param apExtraData The location of the extra data in the table
         * @param aIndentLevel Level of indentation to use
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputEndNode(uint8_t const* const apExtraData, uint32_t const aIndentLevel)
        {
            // No extra data
            IndentOutput(aIndentLevel);
            MiniUART::SendString("};\r\n");
            return apExtraData;
        }

        /**
         * Pretty-prints an unknown value type
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         */
        void PrettyPrintUnknownValue(uint8_t const* const apValue, size_t aLen)
        {
            // Make it pretty clear we don't know what format this is in with <? ?> so it isn't confused with data that
            // might normally be presented as bytes
            MiniUART::SendString("<?");
            auto const* pcurValueByte = apValue;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            for (auto curByte = 0U; curByte != aLen; ++curByte, ++pcurValueByte)
            {
                Print::FormatToMiniUART(" {:x}", *pcurValueByte);
            }
            MiniUART::SendString(" ?>");
        }

        /**
         * Pretty-prints an unsigned 32-bit integer type
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         */
        void PrettyPrintUInt32(uint8_t const* const apValue, size_t aLen)
        {
            if (aLen != sizeof(uint32_t))
            {
                PrettyPrintUnknownValue(apValue, aLen);
            }
            else
            {
                BigEndian<uint32_t> value = 0;
                std::memcpy(&value, apValue, sizeof(value));
                Print::FormatToMiniUART("<{}>", value);
            }
        }

        /**
         * Pretty-prints an unsigned 64-bit integer type
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         */
        /* Currently unused
        void PrettyPrintUInt64(uint8_t const* const apValue, size_t aLen)
        {
            if (aLen != sizeof(uint64_t))
            {
                PrettyPrintUnknownValue(apValue, aLen);
            }
            else
            {
                BigEndian<uint64_t> value = 0;
                std::memcpy(&value, apValue, sizeof(value));
                Print::FormatToMiniUART("<{}>", value);
            }
        }
        */

        /**
         * Pretty-prints a string type
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         */
        void PrettyPrintString(uint8_t const* const apValue, size_t aLen)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            if (aLen != (std::strlen(reinterpret_cast<char const*>(apValue)) + 1))
            {
                PrettyPrintUnknownValue(apValue, aLen);
            }
            else
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                Print::FormatToMiniUART("\"{}\"", reinterpret_cast<char const*>(apValue));
            }
        }

        /**
         * Pretty-prints a phandle type
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         */
        void PrettyPrintPHandle(uint8_t const* const apValue, size_t aLen)
        {
            // These are just visually 32-bit unsigned integers
            PrettyPrintUInt32(apValue, aLen);
        }

        /**
         * Pretty-prints a string list type
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         */
        void PrettyPrintStringList(uint8_t const* const apValue, size_t aLen)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            char const* pcurString = reinterpret_cast<char const*>(apValue);
            size_t curOffset = 0;
            while (curOffset < aLen)
            {
                Print::FormatToMiniUART("{}\"{}\"", (curOffset == 0) ? "" : ", ", pcurString);
                auto const lengthPlusTerminator = std::strlen(pcurString) + 1;
                curOffset += lengthPlusTerminator;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                pcurString += lengthPlusTerminator;
            }
            if (curOffset != aLen)
            {
                MiniUART::SendString(", <BAD STRING LIST>");
            }
        }

        /**
         * Read a cell-sized value (assumed to be 32 or 64 bits in size)
         * 
         * @param apValue The value data
         * @param aCellCount The number of 32 bit cells to read
         * @return The value constructed from the read data
         */
        template<typename ReturnT>
        [[nodiscard]] auto ReadCellSizedValue(uint8_t const* const apValue, uint32_t const aCellCount) -> ReturnT
        {
            uint64_t nativeValue = 0;
            if (aCellCount == 1)
            {
                BigEndian<uint32_t> bevalue = 0;
                std::memcpy(&bevalue, apValue, sizeof(bevalue));
                nativeValue = bevalue;
            }
            else if (aCellCount == 2)
            {
                BigEndian<uint64_t> bevalue = 0;
                std::memcpy(&bevalue, apValue, sizeof(bevalue));
                nativeValue = bevalue;
            }
            // any other sizes are either 0 or not supported
            return ReturnT{ nativeValue };
        }

        /**
         * Pretty-prints a "reg" property
         * 
         * @param apValue Property value data
         * @param aLen Length of the value data
         * @param aCellInfo Cell information needed to parse the data
         */
        void PrettyPrintReg(uint8_t const* const apValue, size_t const aLen, CellInformation const aCellInfo)
        {
            // cells are 32 bits in size, so make sure address and size information is 64 bits or less
            if ((aCellInfo.AddressCells <= 2) && (aCellInfo.SizeCells <= 2))
            {
                // reg is a list of concatinated address and size cell values
                uint8_t const* pcurValue = apValue;
                size_t curOffset = 0;
                while (curOffset < aLen)
                {
                    auto const address = ReadCellSizedValue<PhysicalPtr>(pcurValue, aCellInfo.AddressCells);
                    Print::FormatToMiniUART("{}<{}", (curOffset == 0) ? "" : ", ", address);

                    auto const addressSize = aCellInfo.AddressCells * sizeof(uint32_t);
                    pcurValue += addressSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    curOffset += addressSize;

                    if (aCellInfo.SizeCells > 0)
                    {
                        auto const size = ReadCellSizedValue<size_t>(pcurValue, aCellInfo.SizeCells);
                        Print::FormatToMiniUART(", {}>", size);
                        auto const sizeSize = aCellInfo.AddressCells * sizeof(uint32_t);
                        pcurValue += sizeSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        curOffset += sizeSize;
                    }
                    else
                    {
                        Print::FormatToMiniUART(">");
                    }
                }
                if (curOffset != aLen)
                {
                    MiniUART::SendString(", <BAD ADDR/SIZE PAIR LIST>");
                }
            }
            else
            {
                Print::FormatToMiniUART("<SIZES TOO LARGE> (sizes: {}, {}) ", aCellInfo.AddressCells, aCellInfo.SizeCells);
                PrettyPrintUnknownValue(apValue, aLen);
            }
        }

        /**
         * Tries to pretty-print known property values
         * 
         * @param apName Name of the property
         * @param apValue Property data
         * @param aLen Length of the property data
         * @param aCellInfo The cell information for this property
         */
        void PrettyPrintValue(char const* const apName, uint8_t const* const apValue, size_t const aLen,
            CellInformation const aCellInfo)
        {
            // #TODO: For now we're just handling the common stuff (there's like a better way to do this too)
            if (strcmp(apName, "compatible") == 0)
            {
                PrettyPrintStringList(apValue, aLen);
            }
            else if ((strcmp(apName, "model") == 0) || (strcmp(apName, "status") == 0) || (strcmp(apName, "name") == 0) || (strcmp(apName, "device_type") == 0))
            {
                PrettyPrintString(apValue, aLen);
            }
            else if (strcmp(apName, "phandle") == 0)
            {
                PrettyPrintPHandle(apValue, aLen);
            }
            else if ((strcmp(apName, "#address-cells") == 0) || (strcmp(apName, "#size-cells") == 0) || (strcmp(apName, "virtual-reg") == 0))
            {
                PrettyPrintUInt32(apValue, aLen);
            }
            else if (strcmp(apName, "reg") == 0)
            {
                PrettyPrintReg(apValue, aLen, aCellInfo);
            }
            // #TODO: ranges - see section 2.3.8
            // #TODO: dma-ranges - see section 2.3.9
            // dma-coherent is always empty, so we won't see it
            else
            {
                PrettyPrintUnknownValue(apValue, aLen);
            }
        }

        /**
         * Outputs a property with its extra data
         * 
         * @param aHeader Header containing offsets and other information
         * @param aBaseAddr Base address for offsets in the header
         * @param apExtraData The location of the extra data in the table
         * @param aIndentLevel Level of indentation to use
         * @param arCurStack The current cell information stack
         * @return The new position of the pointer after the extra data and alignment
         */
        uint8_t const* OutputProp(fdt_header const& aHeader, uint8_t const* const aBaseAddr, // NOLINT(bugprone-easily-swappable-parameters)
            uint8_t const* const apExtraData, uint32_t const aIndentLevel, CellInformationStack& arCurStack)
        {
            fdt_prop_extra_data dataHeader;
            std::memcpy(&dataHeader, apExtraData, sizeof(dataHeader));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            uint8_t const* pendPtr = apExtraData + sizeof(dataHeader);

            IndentOutput(aIndentLevel);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            char const* pname = reinterpret_cast<char const*>(aBaseAddr + aHeader.off_dt_strings + dataHeader.nameoff);
            MiniUART::SendString(pname);

            // #TODO: There is likely a far better way to handle this
            if (strcmp(pname, "#address-cells") == 0)
            {
                auto currentCellInfo = arCurStack.Top();
                if (dataHeader.len == sizeof(currentCellInfo.AddressCells))
                {
                    BigEndian<uint32_t> addressCells;
                    std::memcpy(&addressCells, pendPtr, sizeof(currentCellInfo.AddressCells));
                    currentCellInfo.AddressCells = addressCells;
                    arCurStack.SetTop(currentCellInfo);
                }
            }
            else if (strcmp(pname, "#size-cells") == 0)
            {
                auto currentCellInfo = arCurStack.Top();
                if (dataHeader.len == sizeof(currentCellInfo.SizeCells))
                {
                    BigEndian<uint32_t> sizeCells;
                    std::memcpy(&sizeCells, pendPtr, sizeof(currentCellInfo.SizeCells));
                    currentCellInfo.SizeCells = sizeCells;
                    arCurStack.SetTop(currentCellInfo);
                }
            }
            
            if (dataHeader.len == 0)
            {
                MiniUART::SendString(";\r\n");
            }
            else
            {
                MiniUART::SendString(" = ");
                PrettyPrintValue(pname, pendPtr, dataHeader.len, arCurStack.Top());
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                pendPtr += dataHeader.len;
                MiniUART::SendString(";\r\n");
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
            // Nothing to output
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
            // Nothing to output
            return apExtraData;
        }

        /**
         * Outputs the device tree to the UART
         * 
         * @param aHeader The header containing the block offset and other data needed
         * @param aBaseAddr The base address for the offsets
         */
        void OutputDeviceTree(fdt_header const& aHeader, uint8_t const* const aBaseAddr)
        {
            MiniUART::SendString("Structure block:\r\n");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            uint8_t const* pcurToken = aBaseAddr + aHeader.off_dt_struct;
            CellInformationStack infoStack;
            auto indentLevel = 0U;
            auto done = false;
            while (!done)
            {
                BigEndian<uint32_t> token = 0;
                std::memcpy(&token, pcurToken, sizeof(token));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                pcurToken += sizeof(token);

                switch (token)
                {
                case FDT_BEGIN_NODE:
                    {
                        // fine if the stack is empty, as we'll get the defaults
                        auto const parentCellInfo = infoStack.Top();

                        pcurToken = OutputBeginNode(pcurToken, indentLevel);

                        // inherit default values from our parent
                        if (infoStack.Push())
                        {
                            infoStack.SetTop(parentCellInfo);
                        }
                        else
                        {
                            Print::FormatToMiniUART("Out of cell information stack space, aborting\r\n");
                            done = true;
                        }
                    
                        ++indentLevel;
                    }
                    break;

                case FDT_END_NODE:
                    --indentLevel;
                    if (!infoStack.Pop())
                    {
                        Print::FormatToMiniUART("Cell information stack out of sync (too many pops), aborting\r\n");
                        done = true;
                    }
                    pcurToken = OutputEndNode(pcurToken, indentLevel);
                    break;

                case FDT_PROP:
                    if (infoStack.Empty())
                    {
                        Print::FormatToMiniUART("Cell information stack out of sync (empty), aborting\r\n");
                        done = true;
                    }
                    else
                    {
                        pcurToken = OutputProp(aHeader, aBaseAddr, pcurToken, indentLevel, infoStack);
                    }
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

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if ((pcurToken >= (aBaseAddr + aHeader.off_dt_struct + aHeader.size_dt_struct)) && !done)
                {
                    MiniUART::SendString("Ran off the end of the table, aborting\r\n");
                    done = true;
                }
            }
        }
    }

    bool ValidateMagicAndVersion(fdt_header const& aHeader)
    {
        auto valid = (aHeader.magic == ExpectedMagic);
        valid = valid && ((aHeader.version >= ExpectedVersion) && (aHeader.last_comp_version <= ExpectedVersion));
        return valid;
    }

    /**
     * Parse a device tree binary blob
     * 
     * @param apDTB The device tree blob to read
     */
    void ParseDeviceTree(uint8_t const* const apDTB)
    {
        Print::FormatToMiniUART("Loading DTB from: {:x}...\r\n", std::bit_cast<uintptr_t>(apDTB));

        fdt_header header;
        std::memcpy(&header, apDTB, sizeof(header));

        if (header.magic == ExpectedMagic)
        {
            MiniUART::SendString("Magic matches!\r\n");
            if ((header.version >= ExpectedVersion) && (header.last_comp_version <= ExpectedVersion))
            {
                MiniUART::SendString("Version check passes!\r\n");

                OutputHeader(header);
                OutputMemoryReservationMap(header, apDTB);
                OutputDeviceTree(header, apDTB);
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