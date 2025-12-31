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
            // #TODO: Should probably make an iteration helper for this so other systems can reuse the logic
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

        class OutputDebugToUART : public IteratorBase
        {
        public:
            /**
             * Constructor
             * 
             * @param apDTBBase Pointer to the start of the DTB
             */
            explicit OutputDebugToUART(uint8_t const* const apDTBBase)
                : pDTBBase{ apDTBBase }
            {}

            /**
             * Called when the header is read in - header may be invalid!
             * 
             * @param aHeader The header data read in (might be invalid, check status!)
             * @param aValidationStatus The status of the DTB, for error reporting if desired
             * @return Whether iteration should continue
             * @remarks Iteration will stop if validation failed, regardless of the return value of this function
             */
            [[nodiscard]] Result OnHeaderRead(fdt_header const& aHeader, ValidationStatus const aValidationStatus) override
            {
                auto continueIteration = Result::Continue;
                switch (aValidationStatus)
                {
                case ValidationStatus::InvalidMagic:
                    Print::FormatToMiniUART("Magic mismatch, found {:x}, expected {:x}\r\n", aHeader.magic, ExpectedMagic);
                    continueIteration = Result::Stop;
                    break;

                case ValidationStatus::InvalidVersion:
                    Print::FormatToMiniUART("Version check FAILED. Version: {} (last compatible version: {}). Expected version: {}",
                        aHeader.version, aHeader.last_comp_version, ExpectedVersion);
                    continueIteration = Result::Stop;
                    break;

                case ValidationStatus::Valid:
                    MiniUART::SendString("Magic matches, and version check passed.\r\n");
                    OutputHeader(aHeader);
                    OutputMemoryReservationMap(aHeader, pDTBBase);
                    // The block will come next as we enter/leave nodes, so print out a header for it
                    MiniUART::SendString("Structure block:\r\n");
                    break;
                }
                return continueIteration;
            }
            
            /**
             * Called when a node begins
             * 
             * @param apNodeName The name of the node
             * @return Whether iteration should continue
             */
            [[nodiscard]] Result OnBeginNode(char const* const apNodeName) override
            {
                auto continueIteration = Result::Continue;

                // fine if the stack is empty, as we'll get the defaults
                auto const parentCellInfo = InfoStack.Top();

                IndentOutput(IndentLevel);
                Print::FormatToMiniUART("{} {{\r\n", apNodeName);

                // inherit default values from our parent
                if (InfoStack.Push())
                {
                    InfoStack.SetTop(parentCellInfo);
                }
                else
                {
                    Print::FormatToMiniUART("Out of cell information stack space, aborting\r\n");
                    continueIteration = Result::Stop;
                }
            
                ++IndentLevel;
                return continueIteration;
            }

            /**
             * Called when a node ends
             * 
             * @return Whether iteration should continue
             */
            [[nodiscard]] Result OnEndNode() override
            {
                auto continueIteration = Result::Continue;

                --IndentLevel;
                if (!InfoStack.Pop())
                {
                    Print::FormatToMiniUART("Cell information stack out of sync (too many pops), aborting\r\n");
                    continueIteration = Result::Stop;
                }

                // No extra data
                IndentOutput(IndentLevel);
                MiniUART::SendString("};\r\n");

                return continueIteration;
            }

            /**
             * Called when a property is found
             * 
             * @param apPropName The name of the property
             * @param apDataStart The start of the data
             * @param aDataLen The size of the data in bytes
             * @return Whether iteration should continue
             */
            [[nodiscard]] Result OnProperty(char const* const apPropName, uint8_t const* const apDataStart, size_t const aDataLen) override
            {
                auto continueIteration = Result::Continue;
                if (InfoStack.Empty())
                {
                    Print::FormatToMiniUART("Cell information stack out of sync (empty), aborting\r\n");
                    continueIteration = Result::Stop;
                }
                else
                {
                    IndentOutput(IndentLevel);
                    MiniUART::SendString(apPropName);

                    // #TODO: There is likely a far better way to handle this
                    if (strcmp(apPropName, "#address-cells") == 0)
                    {
                        auto currentCellInfo = InfoStack.Top();
                        if (aDataLen == sizeof(currentCellInfo.AddressCells))
                        {
                            BigEndian<uint32_t> addressCells;
                            std::memcpy(&addressCells, apDataStart, sizeof(currentCellInfo.AddressCells));
                            currentCellInfo.AddressCells = addressCells;
                            InfoStack.SetTop(currentCellInfo);
                        }
                    }
                    else if (strcmp(apPropName, "#size-cells") == 0)
                    {
                        auto currentCellInfo = InfoStack.Top();
                        if (aDataLen == sizeof(currentCellInfo.SizeCells))
                        {
                            BigEndian<uint32_t> sizeCells;
                            std::memcpy(&sizeCells, apDataStart, sizeof(currentCellInfo.SizeCells));
                            currentCellInfo.SizeCells = sizeCells;
                            InfoStack.SetTop(currentCellInfo);
                        }
                    }
                    
                    if (aDataLen == 0)
                    {
                        MiniUART::SendString(";\r\n");
                    }
                    else
                    {
                        MiniUART::SendString(" = ");
                        PrettyPrintValue(apPropName, apDataStart, aDataLen, InfoStack.Top());
                        MiniUART::SendString(";\r\n");
                    }
                }
                return continueIteration;
            }

            // We don't do anything special for NOP or End

        private:
            uint8_t const* pDTBBase = nullptr;
            CellInformationStack InfoStack;
            uint32_t IndentLevel = 0U;
        };
    }

    auto IteratorBase::OnHeaderRead(fdt_header const& /*aHeader*/, ValidationStatus const /*aValidationStatus*/) -> Result
    {
        // By default, continue
        return Result::Continue;
    }

    auto IteratorBase::OnBeginNode(char const* const /*apNodeName*/) -> Result
    {
        // By default, continue
        return Result::Continue;
    }

    auto IteratorBase::OnEndNode() -> Result
    {
        // By default, continue
        return Result::Continue;
    }

    auto IteratorBase::OnProperty(char const* const /*apPropName*/, uint8_t const* const /*apDataStart*/, size_t const /*aDataLen*/) -> Result
    {
        // By default, continue
        return Result::Continue;
    }

    auto IteratorBase::OnNOP() -> Result
    {
        // By default, continue
        return Result::Continue;
    }

    void IteratorBase::OnEnd()
    {
        // By default, do nothing
    }

    ValidationStatus ValidateMagicAndVersion(fdt_header const& aHeader)
    {
        if (aHeader.magic == ExpectedMagic)
        {
            if ((aHeader.version >= ExpectedVersion) && (aHeader.last_comp_version <= ExpectedVersion))
            {
                return ValidationStatus::Valid;
            }
            else
            {
                return ValidationStatus::InvalidVersion;
            }
        }
        return ValidationStatus::InvalidMagic;
    }

    bool ParseDeviceTree(uint8_t const* apDTB, IteratorBase& arIterator)
    {
        fdt_header header;
        std::memcpy(&header, apDTB, sizeof(header));
        auto const validationResult = ValidateMagicAndVersion(header);
        auto success = validationResult == ValidationStatus::Valid;

        // We're calling this regardless of validation success so it can handle validation failure with some more
        // information
        // #TODO: Find a better way to do this - probably come up with all the different ways we could fail and make an
        // error result that is bubbled up (std::expected would be nice)
        auto continueIteration = arIterator.OnHeaderRead(header, validationResult);
        if (success && (continueIteration == IteratorBase::Result::Continue))
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto const* pcurToken = apDTB + header.off_dt_struct;
            while (success && (continueIteration == IteratorBase::Result::Continue))
            {
                auto token = BigEndian<uint32_t>{ 0 };
                std::memcpy(&token, pcurToken, sizeof(token));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                pcurToken += sizeof(token);

                switch (token)
                {
                case FDT_BEGIN_NODE:
                    {
                        // our extra data is the node name, as a zero-terminated string
                        
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                        auto const* const pnodeName = reinterpret_cast<char const*>(pcurToken);
                        continueIteration = arIterator.OnBeginNode(pnodeName);

                        auto const nameByteLen = std::strlen(pnodeName) + 1; // including terminator
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        pcurToken = AlignPointer(pcurToken + nameByteLen, alignof(uint32_t));
                    }
                    break;

                case FDT_END_NODE:
                    // no extra data
                    continueIteration = arIterator.OnEndNode();
                    break;

                case FDT_PROP:
                    {
                        auto dataHeader = fdt_prop_extra_data{};
                        std::memcpy(&dataHeader, pcurToken, sizeof(dataHeader));
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        auto const* const pdataStart = pcurToken + sizeof(dataHeader);

                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        auto const* const pname = reinterpret_cast<char const*>(apDTB + header.off_dt_strings + dataHeader.nameoff);
                        
                        continueIteration = arIterator.OnProperty(pname, pdataStart, dataHeader.len);
                        
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        pcurToken = AlignPointer(pdataStart + dataHeader.len, alignof(uint32_t));
                    }
                    break;

                case FDT_NOP:
                    // no extra data
                    continueIteration = arIterator.OnNOP();
                    break;

                case FDT_END:
                    // no extra data
                    arIterator.OnEnd();
                    continueIteration = IteratorBase::Result::Stop;
                    break;

                default:
                    // #TODO: Need better error reporting (invalid token)
                    success = false;
                    break;
                }

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if ((pcurToken >= (apDTB + header.off_dt_struct + header.size_dt_struct)) && success && (continueIteration != IteratorBase::Result::Stop))
                {
                    // #TODO: Need better error reporting (ran off end of memory)
                    success = false;
                }
            }
        }
        return success;
    }

    bool OutputDeviceTreeDebugToUART(uint8_t const* const apDTB)
    {
        Print::FormatToMiniUART("Loading DTB from: {:x}...\r\n", std::bit_cast<uintptr_t>(apDTB));

        auto iterator = OutputDebugToUART{ apDTB };
        return ParseDeviceTree(apDTB, iterator);
    }
}