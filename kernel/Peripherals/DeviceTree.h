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
     * Checks to see if the device tree has the right magic value and a good version
     */
    bool ValidateMagicAndVersion(fdt_header const& aHeader);

    void ParseDeviceTree(uint8_t const* apDTB);
}

#endif // KERNEL_PERIPHERALS_DEVICETREE_H