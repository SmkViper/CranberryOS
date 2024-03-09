// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include "Output.h"

namespace AArch64::Boot
{
    namespace
    {
        /**
         * Halts the CPU (never returns)
        */
        [[noreturn]] void Halt()
        {
            for (;;)
            {
                // wait forever for interupts (which won't happen at this point in the boot process)
                // NOLINTNEXTLINE(hicpp-no-assembler)
                asm volatile("wfi");
            }
        }
    }

    void Panic(char const* /*apMessage*/)
    {
        // #TODO: Actually figure out how to emit this message very early on in the boot process
        Halt();
    }

    void OutputDebug(char const* /*apMessage*/)
    {
        // #TODO: Actually figure out how to emit this message very early on in the boot process
    }
}