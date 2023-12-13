// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include <cstdint>
#include "../RegisterDefines.h"
#include "../SystemRegisters.h"
#include "ExceptionLevel.h"
#include "Output.h"

namespace AArch64
{
    namespace Boot
    {
        namespace ASM
        {
            namespace
            {
                enum class ExceptionLevel : uint8_t
                {
                    EL0 = 0,
                    EL1 = 1,
                    EL2 = 2,
                    EL3 = 3
                };

                /**
                 * Obtains the current exception level we're running under
                 * 
                 * @return The current exception level
                */
                ExceptionLevel GetCurrentExceptionLevel()
                {
                    // #TODO: We have a copy of this in Utils.cpp as well, which we could just re-use, but if we do, I want to
                    // move it to some sort of location where we know it's safe to call from anywhere (including boot)
                    uint64_t exceptionLevel = 0;

                    asm volatile("mrs %[value], CurrentEL" : [value] "=r"(exceptionLevel));

                    // The exception level is in bits 2 and 3 of the value in the CurrentEL register, so extract those and
                    // convert to our enum
                    exceptionLevel = (exceptionLevel >> 2) & 0b11;
                    return static_cast<ExceptionLevel>(exceptionLevel);
                }

                /**
                 * Switches processor exception level from EL2 to EL1
                */
                void SwitchFromEL2ToEL1()
                {
                    // Make sure the stack pointer is copied over as well when switching to EL1, since the processor is set up
                    // with EL1 using its own stack
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

                /**
                 * Sets the Architectural Feature Access Control Register for EL1 to the given value
                 * See: https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/CPACR-EL1--Architectural-Feature-Access-Control-Register
                 * 
                 * @param aValue New value for the register
                */
                void SetCPACR_EL1(uint64_t const aValue)
                {
                    // #TODO: Would be nice to make the value some kind of bitfield or something so it's more readable
                    asm volatile(
                        "msr cpacr_el1, %[value]"
                        : // no outputs
                        :[value] "r"(aValue) // inputs
                        : // no clobbered registers
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
                CPTR_EL2 cptr_el2;
                CPTR_EL2::Write(cptr_el2);
                HSTR_EL2 hstr_el2;
                HSTR_EL2::Write(hstr_el2);

                ASM::SwitchFromEL2ToEL1();
            }
        }

        void SwitchToEL1()
        {
            auto const initialExceptionLevel = AArch64::Boot::ASM::GetCurrentExceptionLevel();

            if (initialExceptionLevel > AArch64::Boot::ASM::ExceptionLevel::EL3)
            {
                Panic("Unknown exception level (above EL3)");
            }
            else if (initialExceptionLevel < AArch64::Boot::ASM::ExceptionLevel::EL1)
            {
                Panic("We must at least be in EL1 to boot");
            }

            if (initialExceptionLevel > AArch64::Boot::ASM::ExceptionLevel::EL2)
            {
                // #TODO: Figure out how to handle EL3
                Panic("We don't yet know how to switch from EL3 to EL2");
            }
            if (initialExceptionLevel > AArch64::Boot::ASM::ExceptionLevel::EL1)
            {
                SwitchFromEL2ToEL1();
            }

            // Disable all traps so that EL1 and EL0 can access the coprocessor, floating point, and SIMD instructions and
            // registers
            ASM::SetCPACR_EL1(CPACR_EL1_INIT_VALUE);
        }
    }
}