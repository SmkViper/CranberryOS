#ifndef KERNEL_AARCH64_BOOT_OUTPUT_H
#define KERNEL_AARCH64_BOOT_OUTPUT_H

namespace AArch64::Boot
{
    /**
     * Panics the kernel with the given message during the boot process (no MMU)
     * 
     * @param apMessage Message to output
     */
    void PanicImpl(char const* apMessage);

    /**
     * Outputs a debug message during the boot process (no MMU)
     * 
     * @param apMessage Message to output
     */
    void OutputDebugImpl(char const* apMessage);

    /**
     * Obtains the current output buffer contents
     */
    char const* GetOutputBuffer();
}

#endif // KERNEL_AARCH64_BOOT_OUTPUT_H