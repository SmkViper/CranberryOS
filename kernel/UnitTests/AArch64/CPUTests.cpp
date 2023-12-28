#include "CPUTests.h"

#include "../../AArch64/CPU.h"
#include "../Framework.h"

namespace UnitTests::AArch64::CPU
{
    namespace
    {
        /**
         * Test to make sure we're running at the expected exception level
         */
        void ExceptionLevelTest()
        {
            EmitTestResult(::AArch64::CPU::GetCurrentExceptionLevel() == ::AArch64::CPU::ExceptionLevel::EL1, "Exception level");
        }

        /**
         * Test to make sure floating point instructions are enabled and working
         */
        void FloatingPointTest()
        {
            // Some slightly complicated code to avoid compiler doing it ahead of time, and motivate it to use
            // the floating point instructions and registers that can trap if not set up right
            const float leftValues[] = {1.5f, 2.6f, 3.7f, 4.8f};
            const float divisor = 2.0f;
            const float expectedValues[] = {0.75f, 1.3f, 1.85f, 2.4f};

            auto success = true;
            for (int i = 0; (i < 4) && success; ++i)
            {
                success = (expectedValues[i] == (leftValues[i] / divisor));
            }
            EmitTestResult(success, "Floating point instructions");
        }

        /**
         * Test to make sure SIMD (NEON) instructions are enabled and working
         */
        void SIMDTest()
        {
            alignas(128) const float leftValues[] = {1.5f, 2.6f, 3.7f, 4.8f};
            alignas(128) const float rightValues[] = {5.5f, 6.6f, 7.7f, 8.8f};
            alignas(128) float results[] = {0.0f, 0.0f, 0.0f, 0.0f};
            // If results is just passed to the inline assembler the compiler can't handle it as an output parameter for
            // an unknown reason (gives a value size warning and a "don't know how to handle tied indirect register
            // inputs" error). Decaying the array to a pointer resolves the issues.
            float* presults = results;

            asm volatile(
                "ld1 {v0.4s}, [%[src1]], #16 \n" // load four floats from src1 into v0
                "ld1 {v1.4s}, [%[src2]], #16 \n" // load four floats from src2 into v1
                "fadd v0.4s, v0.4s, v1.4s \n" // add v0 to v1 and put the results in v0
                "st1 {v0.4s}, [%[dst]], #16 \n" // extract four floats from v0 and put them into dst
                : [dst] "+r" (presults) // output
                : [src1] "r" (leftValues), [src2] "r" (rightValues) // inputs
                : "cc", "memory", "v0", "v1" // clobbered registers
            );

            auto success = true;
            for (int i = 0; (i < 4) && success; ++i)
            {
                success = (results[i] == (leftValues[i] + rightValues[i]));
            }

            EmitTestResult(success, "SIMD Instructions");
        }
    }

    void Run()
    {
        ExceptionLevelTest();
        FloatingPointTest();
        SIMDTest();
    }
}