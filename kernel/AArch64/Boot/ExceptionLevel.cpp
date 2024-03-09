// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include "../CPU.h"
#include "../SystemRegisters.h"
#include "ExceptionLevel.h"
#include "Output.h"

namespace AArch64::Boot
{
    namespace CPU
    {
        namespace
        {
            /**
             * Switches processor exception level from EL2 to EL1
            */
            void SwitchFromEL2ToEL1()
            {
                // Make sure the stack pointer is copied over as well when switching to EL1, since the processor is set up
                // with EL1 using its own stack
                // NOLINTNEXTLINE(hicpp-no-assembler)
                asm volatile(
                            "  mov x0, sp\n"
                            "  msr sp_el1, x0\n"
                            "  adr x0, el1_entry\n" // return to this label at eret
                            "  msr elr_el2, x0\n"
                            "  eret\n"
                            "el1_entry:"
                            : // no outputs
                            : // no inputs
                            : "x0" // clobbered registers
                );
            }
        }
    }

    namespace
    {
        /**
         * Switches from EL2 down to EL1
        */
        void SwitchFromEL2ToEL1()
        {
            HCR_EL2 hcr_el2;
            hcr_el2.RW(true); // Flag EL1 as running in AArch64 mode
            HCR_EL2::Write(hcr_el2);

            SPSR_EL2 spsr_el2;
            spsr_el2.D(true); // Mask debug exceptions
            spsr_el2.A(true); // Mask SError interrupts
            spsr_el2.I(true); // Mask IRQ interrupts
            spsr_el2.F(true); // Mask FIQ interrupts
            spsr_el2.M(SPSR_EL2::Mode::EL1h); // Return to EL1, using SP_EL1 for stack
            SPSR_EL2::Write(spsr_el2);

            // Make sure all traps are disabled so we don't trip up on SIMD or floating point instructions
            CPTR_EL2 const cptr_el2;
            CPTR_EL2::Write(cptr_el2);
            HSTR_EL2 const hstr_el2;
            HSTR_EL2::Write(hstr_el2);

            CPU::SwitchFromEL2ToEL1();
        }
    }

    void SwitchToEL1()
    {
        auto const initialExceptionLevel = AArch64::CPU::GetCurrentExceptionLevel();

        if (initialExceptionLevel > AArch64::CPU::ExceptionLevel::EL3)
        {
            Panic("Unknown exception level (above EL3)");
        }
        else if (initialExceptionLevel < AArch64::CPU::ExceptionLevel::EL1)
        {
            Panic("We must at least be in EL1 to boot");
        }

        if (initialExceptionLevel > AArch64::CPU::ExceptionLevel::EL2)
        {
            // #TODO: Figure out how to handle EL3
            Panic("We don't yet know how to switch from EL3 to EL2");
        }
        if (initialExceptionLevel > AArch64::CPU::ExceptionLevel::EL1)
        {
            SwitchFromEL2ToEL1();
        }

        // Make sure all caches and MMU is disabled, and we're in little endian mode
        SCTLR_EL1 const sctlr_el1;
        SCTLR_EL1::Write(sctlr_el1);

        // Disable all traps so that EL1 and EL0 can access the coprocessor, floating point, and SIMD instructions and
        // registers
        CPACR_EL1 cpacr_el1;
        cpacr_el1.FPEN(CPACR_EL1::FPENTraps::TrapNone);
        CPACR_EL1::Write(cpacr_el1);
    }
}