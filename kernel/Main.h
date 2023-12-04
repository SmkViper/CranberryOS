#ifndef KERNEL_MAIN_H
#define KERNEL_MAIN_H

#include <cstdint>

namespace Kernel
{
    /**
     * Kernel entry point
     * 
     * @param aDTBPointer 32-bit pointer to the Device Tree Binary blob in memory
     * @param aX1Reserved Reserved for future use by the firmware
     * @param aX2Reserved Reserved for future use by the firmware
     * @param aX3Reserved Reserved for future use by the firmware
     * @param aStartPointer 32-bit pointer to _start which the firmware launched
     */
    void kmain(uint32_t const aDTBPointer, uint64_t const aX1Reserved, uint64_t const aX2Reserved,
        uint64_t const aX3Reserved, uint32_t const aStartPointer);
}

#endif // KERNEL_MAIN_H