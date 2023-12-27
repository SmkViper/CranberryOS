#include "UnitTests.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

#include "AArch64/CPU.h"
#include "UnitTests/KernelStdlib/BitsetTests.h"
#include "UnitTests/KernelStdlib/CStringTests.h"
#include "UnitTests/KernelStdlib/ExceptionTests.h"
#include "UnitTests/KernelStdlib/NewTests.h"
#include "UnitTests/KernelStdlib/TypeInfoTests.h"
#include "UnitTests/KernelStdlib/UtilityTests.h"
#include "UnitTests/Framework.h"
#include "UnitTests/MemoryManagerTests.h"
#include "UnitTests/PrintTests.h"
#include "UnitTests/UtilsTests.h"
#include "MemoryManager.h"
#include "Print.h"
#include "Utils.h"

// TODO
// Clean up since this is getting larger

namespace
{
    using namespace ::UnitTests;

    ///////////////////////////////////////////////////////////////////////////
    // Other tests
    ///////////////////////////////////////////////////////////////////////////

    // Using a non-zero, non-one magic numbers here to lower the chance that we're reading garbage memory and passing
    constexpr int StaticObjectInitialized = 10;
    constexpr int StaticObjectDestructed = -10;

    int StaticFunctionTarget = 0;

    /**
     * Ensure C-style "constructor" is called (GCC extension)
     */
    void __attribute__((constructor)) StaticConstructor()
    {
        StaticFunctionTarget = StaticObjectInitialized;
    }

    /**
     * Ensure C-style "destructor" is called (GCC extension)
     */
    void __attribute__((destructor)) StaticDestructor()
    {
        StaticFunctionTarget = StaticObjectDestructed;
    }

    /**
     * Test to ensure static C++ constructors are called
     */
    void StaticCConstructorTest()
    {
        EmitTestResult(StaticFunctionTarget == StaticObjectInitialized, "Static C function construction");
    }

    /**
     * Test to ensure static C++ destructors are called
     */
    void StaticCDestructorTest()
    {
        EmitTestResult(StaticFunctionTarget == StaticObjectDestructed, "Static C function destruction");
    }

    // Dummy test object and static variable to test static init/shutdown
    int StaticObjectTarget = 0;
    struct StaticConstructorDestructorTestObj
    {
        StaticConstructorDestructorTestObj()
        {
            StaticObjectTarget = StaticObjectInitialized;
        }

        ~StaticConstructorDestructorTestObj()
        {
            StaticObjectTarget = StaticObjectDestructed;
        }
    };

    StaticConstructorDestructorTestObj GlobalObject;

    /**
     * Test to ensure static C++ constructors are called
     */
    void StaticCppConstructorTest()
    {
        EmitTestResult(StaticObjectTarget == StaticObjectInitialized, "Static C++ construction");
    }

    /**
     * Test to ensure static C++ destructors are called
     */
    void StaticCppDestructorTest()
    {
        // TODO
        // Clang seems to assume the destructor has no observable side effects, and so does not add
        // it to the .fini_array, causing this test to fail. Need to find a way to subvert clang's
        // analysis (printing to MiniUART and volatile access seem to not be good enough)
        EmitTestResult(StaticObjectTarget == StaticObjectDestructed, "Static C++ destruction");
    }

    /**
     * Test to make sure we're running at the expected exception level
     */
    void ExceptionLevelTest()
    {
        EmitTestResult(AArch64::CPU::GetCurrentExceptionLevel() == AArch64::CPU::ExceptionLevel::EL1, "Exception level");
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
        // #TODO: Disabling test because it breaks later tests (likely the asm block isn't set up right and things are
        // being unexpected clobbered)
        
        /*
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
            : "memory", "v0", "v1" // clobbered registers
        );

        auto success = true;
        for (int i = 0; (i < 4) && success; ++i)
        {
            success = (results[i] == (leftValues[i] + rightValues[i]));
        }

        EmitTestResult(success, "SIMD Instructions");
        */
        EmitTestSkipResult("SIMD Instructions");
    }
}

namespace UnitTests
{
    void Run()
    {
        StaticCConstructorTest();
        StaticCppConstructorTest();
        ExceptionLevelTest();
        FloatingPointTest();
        SIMDTest();

        KernelStdlib::Bitset::Run();
        // No runtime tests for climits
        // No runtime tests for cstddef
        // No runtime tests for cstdint
        KernelStdlib::CString::Run();
        KernelStdlib::Exception::Run();
        KernelStdlib::New::Run();
        KernelStdlib::TypeInfo::Run();
        // No runtime tests for type_traits
        KernelStdlib::Utility::Run();

        MemoryManager::Run();
        Print::Run();
        Utils::Run();
    }

    void RunPostStaticDestructors()
    {
        StaticCDestructorTest();
        StaticCppDestructorTest();
    }
}