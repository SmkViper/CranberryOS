#include <cstdint>
#include "../../Debug.h"
#include "../../Main.h"
#include "../../MemoryManager.h"
#include "../../PointerTypes.h"
#include "ExceptionLevel.h"
#include "MMU.h"
//#include "Output.h"

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
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void boot_kernel(uint32_t const aDTBPointer, uint64_t const aX1Reserved, uint64_t const aX2Reserved,
        uint64_t const aX3Reserved, uint32_t const aStartPointer)
    {
        // #TODO: So we have this working in QEMU, but it still doesn't work on real hardware.
        // Did a bunch more experimentation and it seems that we can output a string up to 7 characters long (plus null
        // terminator), but one that is 8 + terminator causes real hardware to halt. I.e. Debug::OutputDebug("Test567")
        // works just fine before all the code below, but Debug::OutputDebug("Test5678") does not. I confirmed that
        // reading the data in the string works just fine (strlen doesn't halt), but the memcpy to the buffer does not.
        //
        // Reproducing the memcpy loop here in testing reproduces the halt on real hardware, but adding an if check
        // (even if it never passes) makes the memcpy loop work, which implies to me that the compiler might be doing
        // some tricky optimizations using instructions that are invalid until we've done all this initial setup work.
        // I initially thought it was doing some SIMD work (which isn't turned on until SwitchToEL1 returns) but none
        // of the three outputs below will work, including the ones after SIMD is turned on) so that isn't it. (Or
        // there is more than one issue that is causing the halt that I haven't found yet)
        
        //Debug::OutputDebug("Switching to EL1...");
        AArch64::Boot::SwitchToEL1();

        // have to store this before the MMU is turned on and we lose access to the physical address
        auto const deviceTreeVA = AArch64::Boot::StoreFlattenedDeviceTree(PhysicalPtr{ aDTBPointer });

        //Debug::OutputDebug("Setting up page tables...");
        AArch64::Boot::InitPageTablesAndMMU();

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

        // #TODO: This technically appears to work, in that it unmaps the memory, but that seems to cause problems with
        // our debug out functions which trap when trying to access their function pointers for unknown reasons. I
        // really have to track down what is going on with static variable access during boot, especially since
        // sometimes it appears to be physical, sometimes it appears to be virtual, sometimes it's PC-relative, and it 
        // even seems to depend on debug vs release
        //AArch64::Boot::UnmapIdentityMapping();

        Debug::OutputDebug("Calling kmain()...");
        Kernel::kmain(deviceTreeVA, aX1Reserved, aX2Reserved, aX3Reserved, PhysicalPtr{ aStartPointer });
    }
}