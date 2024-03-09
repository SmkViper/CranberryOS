#include <cstdint>
#include "../../Main.h"
#include "../../MemoryManager.h"
#include "../../PointerTypes.h"
#include "ExceptionLevel.h"
#include "MMU.h"

extern "C"
{
    /**
     * Called from assembly to set up everything the kernel needs to boot, and then jumps into kmain.
     * 
     * @param aDTBPointer 32-bit pointer to the Device Tree Binary blob in memory
     * @param aX1Reserved Reserved for future use by the firmware
     * @param aX2Reserved Reserved for future use by the firmware
     * @param aX3Reserved Reserved for future use by the firmware
     * @param aStartPointer 32-bit pointer to _start which the firmware launched
    */
    void boot_kernel(uint32_t const aDTBPointer, uint64_t const aX1Reserved, uint64_t const aX2Reserved,
        uint64_t const aX3Reserved, uint32_t const aStartPointer)
    {
        AArch64::Boot::SwitchToEL1();
        AArch64::Boot::CreatePageTables();
        AArch64::Boot::EnableMMU();

        // The MMU is now on, but our stack pointer and instruction pointer are still pointing at the original physical
        // addresses, which are identity mapped. We need to move those to the kernel virtual addresses, so we can clean
        // up the identity mapping.

        // Use an absolute jump to move to the kernel address space
        // NOLINTNEXTLINE(hicpp-no-assembler)
        asm volatile(
            "ldr x0, =1f \n"
            "br x0 \n"
            "1: \n"
            : // no outputs
            : // no inputs
            : "x0" // bashed registers
        );

        // Adjust the stack pointer by KernalVirtualAddressStart so it points into kernel space
        // NOLINTNEXTLINE(hicpp-no-assembler)
        asm volatile(
            "mov x0, %[base] \n"
            "add sp, sp, x0 \n"
            : // no outputs
            : [base] "r"(MemoryManager::KernelVirtualAddressOffset)
            : "x0" // bashed registers
        );

        // #TODO: Unmap identity mapping

        Kernel::kmain(PhysicalPtr{ aDTBPointer }, aX1Reserved, aX2Reserved, aX3Reserved, PhysicalPtr{ aStartPointer });
    }
}