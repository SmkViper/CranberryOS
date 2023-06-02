#ifndef KERNEL_PERIPHERALS_DEVICETREE_H
#define KERNEL_PERIPHERALS_DEVICETREE_H

#include <cstdint>

namespace DeviceTree
{
    void ParseDeviceTree(uint8_t const* apDTB);
}

#endif // KERNEL_PERIPHERALS_DEVICETREE_H