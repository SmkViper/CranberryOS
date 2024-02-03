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
            const float leftValues[] = {1.5F, 2.6F, 3.7F, 4.8F}; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            const float divisor = 2.0F;
            const float expectedValues[] = {0.75F, 1.3F, 1.85F, 2.4F}; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

            auto success = true;
            for (int i = 0; (i < 4) && success; ++i)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                success = (expectedValues[i] == (leftValues[i] / divisor));
            }
            EmitTestResult(success, "Floating point instructions");
        }

        /**
         * Test to make sure SIMD (NEON) instructions are enabled and working
         */
        void SIMDTest()
        {
            constexpr auto SIMDAlign = 128;
            alignas(SIMDAlign) float const leftValues[] = {1.5F, 2.6F, 3.7F, 4.8F}; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            alignas(SIMDAlign) float const rightValues[] = {5.5F, 6.6F, 7.7F, 8.8F}; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            alignas(SIMDAlign) float results[] = {0.0F, 0.0F, 0.0F, 0.0F}; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            // If results is just passed to the inline assembler the compiler can't handle it as an output parameter for
            // an unknown reason (gives a value size warning and a "don't know how to handle tied indirect register
            // inputs" error). Decaying the array to a pointer resolves the issues.
            auto* presults = static_cast<float*>(results);

            // NOLINTNEXTLINE(hicpp-no-assembler)
            asm volatile(
                "ld1 {v0.4s}, [%[src1]], #16 \n" // load four floats from src1 into v0
                "ld1 {v1.4s}, [%[src2]], #16 \n" // load four floats from src2 into v1
                "fadd v0.4s, v0.4s, v1.4s \n" // add v0 to v1 and put the results in v0
                "st1 {v0.4s}, [%[dst]], #16 \n" // extract four floats from v0 and put them into dst
                : [dst] "+r" (presults) // output
                : [src1] "r" (static_cast<float const*>(leftValues)), [src2] "r" (static_cast<float const*>(rightValues)) // inputs
                : "cc", "memory", "v0", "v1" // clobbered registers
            );

            auto success = true;
            for (int i = 0; (i < 4) && success; ++i)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
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