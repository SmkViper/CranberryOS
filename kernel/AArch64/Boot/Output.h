#ifndef KERNEL_AARCH64_BOOT_OUTPUT_H
#define KERNEL_AARCH64_BOOT_OUTPUT_H

namespace AArch64::Boot
{
    // #TODO: Want to figure out a way to help ensure that the NoMMU versions are never used after MMU is set up, and
    // the MMU versions aren't used BEFORE the MMU is set up.
    // Probably going to need to seperate out the code a little more into pre- and post-MMU functions/files, and
    // ensuring that they online include pre- or post-MMU headers
    
    /**
     * Panics the kernel with the given message during the boot process (no MMU)
     * 
     * @param apMessage Message to output
     */
    void PanicNoMMU(char const* apMessage);

    /**
     * Panics the kernel with the given message during the boot process (MMU available)
     * 
     * @param apMessage Message to output
     */
    void PanicMMU(char const* apMessage);

    /**
     * Outputs a debug message during the boot process (no MMU)
     * 
     * @param apMessage Message to output
     */
    void OutputDebugNoMMU(char const* apMessage);

    /**
     * Outputs a debug message during the boot process (MMU available)
     * 
     * @param apMessage Message to output
     */
    void OutputDebugMMU(char const* apMessage);
    
    /**
     * Obtains the current output buffer contents (assumes MMU is on)
     */
    char const* GetOutputBuffer();
}

#endif // KERNEL_AARCH64_BOOT_OUTPUT_H