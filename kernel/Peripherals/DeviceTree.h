#ifndef KERNEL_PERIPHERALS_DEVICETREE_H
#define KERNEL_PERIPHERALS_DEVICETREE_H

#include <cstdint>
#include "../BigEndian.h"

namespace DeviceTree
{
    // From DeviceTree specification, section 5.2
    struct fdt_header
    {
        // NOTE: All values are big-endian when loaded from memory
        BigEndian<uint32_t> magic = 0; // "Magic" number to verify the header is valid
        BigEndian<uint32_t> totalsize = 0; // The total size of the device tree blob, including all padding
        BigEndian<uint32_t> off_dt_struct = 0; // Offset to the structure block from the header, in bytes
        BigEndian<uint32_t> off_dt_strings = 0; // Offset to the strings block from the header, in bytes
        BigEndian<uint32_t> off_mem_rsvmap = 0; // Offset to the memory reservation block from the header, in bytes
        BigEndian<uint32_t> version = 0; // The version of the data structure
        BigEndian<uint32_t> last_comp_version = 0; // The lowest version which this structure is backwards compatible with
        BigEndian<uint32_t> boot_cpuid_phys = 0; // Physical ID of the boot CPU. Same as the "reg" property of the CPU node
        BigEndian<uint32_t> size_dt_strings = 0; // Length in bytes of the strings block
        BigEndian<uint32_t> size_dt_struct = 0; // Length in bytes of the structs block
    };

    /**
     * The status of header validation
     */
    enum class ValidationStatus : int8_t
    {
        Valid,
        InvalidMagic,
        InvalidVersion
    };

    /**
     * Base class for iterating a device tree - virtuals will be called at various points in the iteration process
     * #TODO: Should probably have a const version of this
     */
    class IteratorBase
    {
    public:
        /**
         * Default constructor
         */
        IteratorBase() = default;

        /**
         * Destructor
         */
        virtual ~IteratorBase() = default;

        // Disable copy and move
        IteratorBase(IteratorBase const&) = delete;
        IteratorBase(IteratorBase&&) = delete;
        IteratorBase& operator=(IteratorBase const&) = delete;
        IteratorBase& operator=(IteratorBase&&) = delete;

        /**
         * Returned by each callback function to indicate whether iteration should continue or not
         */
        enum class Result : int8_t { Continue, Stop };

        // #TODO: Should probably have these return some kind of std::expected<IterationResult> so errors can be
        // propigated better

        /**
         * Called when the header is read in - header may be invalid!
         * 
         * @param aHeader The header data read in (might be invalid, check status!)
         * @param aValidationStatus The status of the DTB, for error reporting if desired
         * @return Whether iteration should continue
         * @remarks Iteration will stop if validation failed, regardless of the return value of this function
         */
        [[nodiscard]] virtual Result OnHeaderRead(fdt_header const& aHeader, ValidationStatus aValidationStatus);

        // #TODO: Should probably use string_view for the names when we have it

        /**
         * Called when a node begins
         * 
         * @param apNodeName The name of the node
         * @return Whether iteration should continue
         */
        [[nodiscard]] virtual Result OnBeginNode(char const* apNodeName);

        /**
         * Called when a node ends
         * 
         * @return Whether iteration should continue
         */
        [[nodiscard]] virtual Result OnEndNode();

        // #TODO: We should probably have a span for the data

        /**
         * Called when a property is found
         * 
         * @param apPropName The name of the property
         * @param apDataStart The start of the data
         * @param aDataLen The size of the data in bytes
         * @return Whether iteration should continue
         */
        [[nodiscard]] virtual Result OnProperty(char const* apPropName, uint8_t const* apDataStart, size_t aDataLen);

        /**
         * Called when a NOP is found
         * 
         * @return Whether iteration should continue
         */
        [[nodiscard]] virtual Result OnNOP();

        /**
         * Called when the end of the table is found
         */
        virtual void OnEnd();
    };

    // #TODO: Change our one caller to use ParseDeviceTree and only handle OnHeaderRead
    /**
     * Checks to see if the device tree has the right magic value and a good version
     * 
     * @param aHeader The header to validate
     */
    [[nodiscard]] ValidationStatus ValidateMagicAndVersion(fdt_header const& aHeader);

    /**
     * Parses a device tree, calling the callbacks it is given
     * 
     * @param apDTB The device tree blob start pointer
     * @param arIterator The iterator to call back as iteration progresses
     * @return Whether parsing succeeded or not
     */
    [[nodiscard]] bool ParseDeviceTree(uint8_t const* apDTB, IteratorBase& arIterator);

    // #TODO: We should probably have an override that takes a const iterator

    /**
     * Outputs the device tree to UART for debugging
     * 
     * @param apDTB The device tree blob start pointer
     * @return Whether output succeeded or not
     */
    [[nodiscard]] bool OutputDeviceTreeDebugToUART(uint8_t const* apDTB);
}

#endif // KERNEL_PERIPHERALS_DEVICETREE_H