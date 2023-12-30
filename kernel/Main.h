#ifndef KERNEL_MAIN_H
#define KERNEL_MAIN_H

#include <cstdint>
#include "PointerTypes.h"

namespace Kernel
{
    /**
     * Kernel entry point
     * 
     * @param aDTBPointer Pointer to the Device Tree Binary blob in memory
     * @param aX1Reserved Reserved for future use by the firmware
     * @param aX2Reserved Reserved for future use by the firmware
     * @param aX3Reserved Reserved for future use by the firmware
     * @param aStartPointer Pointer to _start which the firmware launched
     */
    void kmain(PhysicalPtr aDTBPointer, uint64_t aX1Reserved, uint64_t aX2Reserved,
        uint64_t aX3Reserved, PhysicalPtr aStartPointer);
}

#endif // KERNEL_MAIN_H