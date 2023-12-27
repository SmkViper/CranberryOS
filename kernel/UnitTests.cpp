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
#include "UnitTests/Framework.h"
#include "MemoryManager.h"
#include "Print.h"
#include "Utils.h"

// TODO
// Clean up since this is getting larger

namespace
{
    using namespace ::UnitTests;

    ///////////////////////////////////////////////////////////////////////////
    // utility tests
    ///////////////////////////////////////////////////////////////////////////

    struct MoveCopyCountStruct
    {
        MoveCopyCountStruct() = default;
        MoveCopyCountStruct([[maybe_unused]] const MoveCopyCountStruct& aOther): CopyCount{1} {}
        MoveCopyCountStruct([[maybe_unused]] MoveCopyCountStruct&& amOther): MoveCount{1} {}
        MoveCopyCountStruct& operator=([[maybe_unused]] const MoveCopyCountStruct& aOther)
        {
            ++CopyCount;
            return *this;
        }
        MoveCopyCountStruct& operator=([[maybe_unused]] MoveCopyCountStruct&& amOther)
        {
            ++MoveCount;
            return *this;
        }

        uint32_t MoveCount = 0;
        uint32_t CopyCount = 0;
    };

    /**
     * Ensure std::move moves the type
     */
    void StdMoveTest()
    {
        MoveCopyCountStruct source;
        auto dest = MoveCopyCountStruct{std::move(source)};

        EmitTestResult(dest.MoveCount == 1, "std::move");
    }

    /**
     * Helper to get us a forwarding ref (requires templates - templated lamdas are in C++20)
     * 
     * @param aForwardingRef The forwarding ref to forward to the destination object
     * @param aResultFunctor The functor to call with the destination object
     */
    template<typename T, typename FunctorType>
    void StdForwardTestHelper(T&& aForwardingRef, const FunctorType& aResultFunctor)
    {
        auto dest = MoveCopyCountStruct{std::forward<T>(aForwardingRef)};
        aResultFunctor(dest);
    }

    /**
     * Ensure std::forward forwards the type
     */
    void StdForwardTest()
    {
        MoveCopyCountStruct lvalueSource;
        StdForwardTestHelper(lvalueSource, [](const MoveCopyCountStruct& aDest)
        {
            EmitTestResult(aDest.CopyCount == 1, "std::forward lvalue copy");
        });
        StdForwardTestHelper(std::move(lvalueSource), [](const MoveCopyCountStruct& aDest)
        {
            EmitTestResult(aDest.MoveCount == 1, "std::forward rvalue move");
        });
    }

    ///////////////////////////////////////////////////////////////////////////
    // MemoryManager.h tests
    ///////////////////////////////////////////////////////////////////////////

    static_assert(MemoryManager::CalculateBlockStart(0x1, 0x1000) == 0x0, "Unexpected block start");
    static_assert(MemoryManager::CalculateBlockEnd(0x1, 0x1000) == 0x0FFF, "Unexpected block end");
    static_assert(MemoryManager::CalculateBlockStart(0x1024, 0x1000) == 0x1000, "Unexpected block start");
    static_assert(MemoryManager::CalculateBlockEnd(0x1024, 0x1000) == 0x1FFF, "Unexpected block end");

    ///////////////////////////////////////////////////////////////////////////
    // Print.h tests
    ///////////////////////////////////////////////////////////////////////////

    // We can't really "test" output to MiniUART (since we can't read what we output) but we can test the buffer output

    /**
     * Ensure printing to a buffer with no arguments works
     */
    void PrintNoArgsTest()
    {
        const char expectedOutput[] = "Hello World";
        char buffer[256];
        Print::FormatToBuffer(buffer, expectedOutput);

        EmitTestResult(strcmp(buffer, expectedOutput) == 0, "Print::FormatToBuffer with no args");
    }

    /**
     * Ensure printing to a buffer with no arguments and a too-small buffer works
     */
    void PrintNoArgsTruncatedBufferTest()
    {
        const char expectedOutput[] = "Hello World";
        char buffer[5];
        Print::FormatToBuffer(buffer, expectedOutput);

        EmitTestResult(strcmp(buffer, "Hell") == 0, "Print::FormatToBuffer with no args and a too-small buffer");
    }

    /**
     * Ensure printing to a buffer with string arguments (both raw string and pointer) works
     */
    void PrintStringArgsTest()
    {
        const char* pstringPointer = "Again";
        char buffer[256];
        Print::FormatToBuffer(buffer, "Hello {} {}", "World", pstringPointer);
        EmitTestResult(strcmp(buffer, "Hello World Again") == 0, "Print::FormatToBuffer with string arguments");
    }

    /**
     * Ensure printing to a buffer with string arguments (both raw string and pointer) and a too-small buffer works
     */
    void PrintStringArgsTruncatedBufferTest()
    {
        const char* pstringPointer = "Again";
        char buffer[8];
        Print::FormatToBuffer(buffer, "Hello {} {}", "World", pstringPointer);
        EmitTestResult(strcmp(buffer, "Hello W") == 0, "Print::FormatToBuffer with string arguments and a too-small buffer");
    }

    /**
     * Ensure printing to a buffer with integer arguments works
     */
    void PrintIntegerArgsTest()
    {
        char buffer[256];
        Print::FormatToBuffer(buffer, "Test {}, test {}, test {}", 1u, 102u, 0u);
        EmitTestResult(strcmp(buffer, "Test 1, test 102, test 0") == 0, "Print::FormatToBuffer with integer arguments");

        Print::FormatToBuffer(buffer, "Format Test {:}", 1u);
        EmitTestResult(strcmp(buffer, "Format Test 1") == 0, "Print::FormatToBuffer with integer arguments and empty format string");

        constexpr auto testBinaryNumber = 0b1100'1010u;
        Print::FormatToBuffer(buffer, "Binary Test {:b} {:B}", testBinaryNumber, testBinaryNumber);
        EmitTestResult(strcmp(buffer, "Binary Test 0b11001010 0B11001010") == 0, "Print::FormatToBuffer with integer arguments and binary format string");

        constexpr auto testOctalNumber = 0123u;
        Print::FormatToBuffer(buffer, "Octal Test {:o} {:o}", testOctalNumber, 0u);
        EmitTestResult(strcmp(buffer, "Octal Test 0123 0") == 0, "Print::FormatToBuffer with integer arguments and octal format string");

        constexpr auto testDecimalNumber = 123u;
        Print::FormatToBuffer(buffer, "Decimal Test {:d}", testDecimalNumber, testDecimalNumber);
        EmitTestResult(strcmp(buffer, "Decimal Test 123") == 0, "Print::FormatToBuffer with integer arguments and decimal format string");

        constexpr auto testHexNumber = 0x11ff89abu;
        Print::FormatToBuffer(buffer, "Hex Test {:x} {:X}", testHexNumber, testHexNumber);
        EmitTestResult(strcmp(buffer, "Hex Test 0x11ff89ab 0X11FF89AB") == 0, "Print::FormatToBuffer with integer arguments and hex format string");
    }

    /**
     * Ensure printing to a buffer with integer arguments and a too-small buffer works
     */
    void PrintIntegerArgsTruncatedBufferTest()
    {
        char buffer[15];
        Print::FormatToBuffer(buffer, "Test {}, test {}", 1u, 102u);
        EmitTestResult(strcmp(buffer, "Test 1, test 1") == 0, "Print::FormatToBuffer with integer arguments and a too-small buffer");
    }

    /**
     * Ensure escaped braces are printed
     */
    void PrintEscapedBracesTest()
    {
        char buffer[256];
        Print::FormatToBuffer(buffer, "Open {{ close }}");
        EmitTestResult(strcmp(buffer, "Open { close }") == 0, "Print::FormatToBuffer escaped braces");
    }

    /**
     * Ensure mismatched braces are handled
     */
    void PrintMismatchedBracesTest()
    {
        char buffer[256];
        Print::FormatToBuffer(buffer, "Close } some other text");
        EmitTestResult(strcmp(buffer, "Close ") == 0, "Print::FormatToBuffer mismatched close brace");

        Print::FormatToBuffer(buffer, "Open { some other text");
        EmitTestResult(strcmp(buffer, "Open ") == 0, "Print::FormatToBuffer mismatched open brace");
    }

    /**
     * Ensure invalid brace contents are handled
     */
    void PrintInvalidBraceContentsTest()
    {
        char buffer[256];
        Print::FormatToBuffer(buffer, "Hello {some bad text} world", "bad");
        EmitTestResult(strcmp(buffer, "Hello bad world") == 0, "Print::FormatToBuffer invalid brace contents");
    }

    /**
     * Ensure out of range braces are handled
     */
    void PrintOutOfRangeBracesTest()
    {
        char buffer[256];
        Print::FormatToBuffer(buffer, "Hello {} world {} again", "new");
        EmitTestResult(strcmp(buffer, "Hello new world {1} again") == 0, "Print::FormatToBuffer out of range braces");
    }

    ///////////////////////////////////////////////////////////////////////////
    // Utils.h tests
    ///////////////////////////////////////////////////////////////////////////

    // #TODO: no obvious way to test MemoryMappedIO functions at this time
    // #TODO: no obvious way to test Timing functions at this time

    /**
     * Ensure WriteMultiBitValue and ReadMultiBitValue set and read the expected bits
    */
    void ReadWriteMultiBitValueTest()
    {
        static constexpr uint64_t inputValue = 0xABCD; // larger than the mask to ensure we're masking
        static constexpr uint64_t mask = 0xFF;
        static constexpr uint64_t shift = 3;

        std::bitset<64> bitset;
        WriteMultiBitValue(bitset, inputValue, mask, shift);
        EmitTestResult(bitset.to_ullong() == 0x0000'0000'0000'0668, "Write multi bit value mask/shift");
        EmitTestResult(ReadMultiBitValue<uint64_t>(bitset, mask, shift) == 0xCD, "Read/write multi bit value round-trip");
    }

    /**
     * Ensure WriteMultiBitValue and ReadMultiBitValue work with enum values
    */
    void ReadWriteMultiBitEnumTest()
    {
        enum class TestEnum: uint8_t
        {
            Value = 0b101
        };
        static constexpr uint64_t mask = 0b111;
        static constexpr uint64_t shift = 3;

        std::bitset<64> bitset;
        WriteMultiBitValue(bitset, TestEnum::Value, mask, shift);
        EmitTestResult(bitset.to_ullong() == 0x0000'0000'0000'0028, "Write multi bit enum mask/shift");
        EmitTestResult(ReadMultiBitValue<TestEnum>(bitset, mask, shift) == TestEnum::Value, "Read/write multi bit enum round-trip");
    }

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
        
        StdMoveTest();
        StdForwardTest();
        
        PrintNoArgsTest();
        PrintNoArgsTruncatedBufferTest();
        PrintStringArgsTest();
        PrintStringArgsTruncatedBufferTest();
        PrintIntegerArgsTest();
        PrintIntegerArgsTruncatedBufferTest();
        PrintEscapedBracesTest();
        PrintMismatchedBracesTest();
        PrintInvalidBraceContentsTest();
        PrintOutOfRangeBracesTest();

        ReadWriteMultiBitValueTest();
        ReadWriteMultiBitEnumTest();
    }

    void RunPostStaticDestructors()
    {
        StaticCDestructorTest();
        StaticCppDestructorTest();
    }
}