#ifndef KERNEL_ARM_BOOT_MMU_H
#define KERNEL_ARM_BOOT_MMU_H

namespace AArch64
{
    namespace Boot
    {
        /**
         * Sets up the page tables needed for booting
        */
        void CreatePageTables();

        /**
         * Turns on the memory management unit
        */
        void EnableMMU();

        // #TODO: Going to want to have a way to unmap the identity mapping once we no longer need it
    }
}

#endif // KERNEL_ARM_BOOT_MMU_H