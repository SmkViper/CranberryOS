#ifndef KERNEL_AARCH64_BOOT_EXCEPTIONLEVEL_H
#define KERNEL_AARCH64_BOOT_EXCEPTIONLEVEL_H

namespace AArch64
{
    namespace Boot
    {
        /**
         * Switches the processer into EL1.
        */
        void SwitchToEL1();
    }
}

#endif // KERNEL_AARCH64_BOOT_EXCEPTIONLEVEL_H