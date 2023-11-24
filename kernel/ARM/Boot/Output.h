#ifndef KERNEL_ARM_BOOT_OUTPUT_H
#define KERNEL_ARM_BOOT_OUTPUT_H

namespace AArch64
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

#endif // KERNEL_ARM_BOOT_OUTPUT_H