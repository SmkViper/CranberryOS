// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include <cstdint>
#include "../SystemRegisters.h"
#include "Output.h"

namespace AArch64
{
    namespace Boot
    {
        namespace ASM
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
             * Sets the Hypervisor Configuration Register for EL2 to the given value
             * See: https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register
             * 
             * @param aValue New value for the register
            */
            void SetHCR_EL2(uint64_t const aValue)
            {
                // #TODO: Would be nice to make the value some kind of bitfield or something so it's more readable
                asm volatile(
                    "msr hcr_el2, %[value]"
                    : // no outputs
                    :[value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }

            /**
             * Sets the Saved Program Status Register for EL2 to the given value
             * See: https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-
             * 
             * @param aValue New value for the register
            */
            void SetSPSR_EL2(uint64_t const aValue)
            {
                // #TODO: Would be nice to make the value some kind of bitfield or something so it's more readable
                asm volatile(
                    "msr spsr_el2, %[value]"
                    : // no outputs
                    :[value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }

            /**
             * Sets the Architectural Feature Trap Register for EL2 to the given value
             * See: https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/CPTR-EL2--Architectural-Feature-Trap-Register--EL2-
             * 
             * @param aValue New value for the register
            */
            void SetCPTR_EL2(uint64_t const aValue)
            {
                // #TODO: Would be nice to make the value some kind of bitfield or something so it's more readable
                asm volatile(
                    "msr cptr_el2, %[value]"
                    : // no outputs
                    :[value] "r"(aValue) // inputs
                    : // no clobbered registers
                );
            }

            /**
             * Sets the Hypervisor System Tracp Register for EL2 to the given value
             * See: https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/HSTR-EL2--Hypervisor-System-Trap-Register
             * 
             * @param aValue New value for the register
            */
            void SetHSTR_EL2(uint64_t const aValue)
            {
                // #TODO: Would be nice to make the value some kind of bitfield or something so it's more readable
                asm volatile(
                    "msr hstr_el2, %[value]"
                    : // no outputs
                    :[value] "r"(aValue) // inputs
                    : // no clobbered registers
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

        /**
         * Switches from EL2 down to EL1
        */
        void SwitchFromEL2ToEL1()
        {
            // See SystemRegisters.h for these values
            ASM::SetHCR_EL2(HCR_EL2_INIT_VALUE);
            ASM::SetSPSR_EL2(SPSR_EL2_INIT_VALUE);

            // Disable all traps so that EL2, EL1, and EL0 can access the coprocessor, floating point, and SIMD
            // instructions and registers
            ASM::SetCPTR_EL2(CPTR_EL2_INIT_VALUE);
            ASM::SetHSTR_EL2(HSTR_EL2_INIT_VALUE);

            ASM::SwitchFromEL2ToEL1();
        }
    }
}

extern "C"
{
    /**
     * Called from assembly to switch from our current exception level down to EL1 so we can boot
    */
    void switch_to_el1()
    {
        auto const initialExceptionLevel = AArch64::Boot::ASM::GetCurrentExceptionLevel();

        if (initialExceptionLevel > AArch64::Boot::ASM::ExceptionLevel::EL3)
        {
            AArch64::Boot::Panic("Unknown exception level (above EL3)");
        }
        else if (initialExceptionLevel < AArch64::Boot::ASM::ExceptionLevel::EL1)
        {
            AArch64::Boot::Panic("We must at least be in EL1 to boot");
        }

        if (initialExceptionLevel > AArch64::Boot::ASM::ExceptionLevel::EL2)
        {
            // #TODO: Figure out how to handle EL3
            AArch64::Boot::Panic("We don't yet know how to switch from EL3 to EL2");
        }
        if (initialExceptionLevel > AArch64::Boot::ASM::ExceptionLevel::EL1)
        {
            AArch64::Boot::SwitchFromEL2ToEL1();
        }

        // Disable all traps so that EL1 and EL0 can access the coprocessor, floating point, and SIMD instructions and
        // registers
        AArch64::Boot::ASM::SetCPACR_EL1(CPACR_EL1_INIT_VALUE);
    }
}