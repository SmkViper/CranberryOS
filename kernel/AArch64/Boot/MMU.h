#ifndef KERNEL_AARCH64_BOOT_MMU_H
#define KERNEL_AARCH64_BOOT_MMU_H

#include "../../PointerTypes.h"

namespace AArch64::Boot
{
    /**
     * Sets up the page tables needed for booting
     */
    void CreatePageTables();

    /**
     * Turns on the memory management unit
     */
    void EnableMMU();

    /**
     * Stores the device tree into a known location and returns where it was copied to
     * Expected to be called before the MMU is turned on
     * 
     * @param aDeviceTree Where the bootloader put the device tree
     * @return Where we moved the device tree
     */
    VirtualPtr StoreFlattenedDeviceTree(PhysicalPtr aDeviceTree);

    /**
     * Clear out the identity mapping that was used for booting
     */
    void UnmapIdentityMapping();
}

#endif // KERNEL_AARCH64_BOOT_MMU_H