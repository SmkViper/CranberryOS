#ifndef KERNEL_AARCH64_BOOT_OUTPUT_H
#define KERNEL_AARCH64_BOOT_OUTPUT_H

namespace AArch64
{
    namespace Boot
    {
        /**
         * Panics the kernel with the given message during the boot process (no MMU)
         * 
         * @param apMessage Message to output
        */
        void Panic(char const* apMessage);

        /**
         * Outputs a debug message during the boot process (no MMU)
         * 
         * @param apMessage Message to output
        */
        void OutputDebug(char const* apMessage);
    }
}

#endif // KERNEL_AARCH64_BOOT_OUTPUT_H