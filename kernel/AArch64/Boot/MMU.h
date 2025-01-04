#ifndef KERNEL_AARCH64_BOOT_MMU_H
#define KERNEL_AARCH64_BOOT_MMU_H

#include <bit>
#include "../../PointerTypes.h"

namespace AArch64::Boot
{
    /**
     * Sets up the page tables needed for booting and turns on the MMU
     */
    void InitPageTablesAndMMU();

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

    /**
     * Adjusts a pointer that might be the upper half (i.e. globals) to be accessed without the MMU
     * 
     * @param apPtr The pointer to adjust
     * @return The adjusted pointer
     */
    uintptr_t AdjustPointerForNoMMU(uintptr_t aPtr);

    /**
     * Adjusts a global reference to account for no MMU
     * 
     * @param aGlobal Global we want to access
     * @return A reference to the global
     */
    template<typename T>
    T& GlobalNoMMU(T& aGlobal)
    {
        return *std::bit_cast<T*>(AdjustPointerForNoMMU(std::bit_cast<uintptr_t>(&aGlobal)));
    }
}

#endif // KERNEL_AARCH64_BOOT_MMU_H