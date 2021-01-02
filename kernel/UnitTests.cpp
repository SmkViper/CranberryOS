#include "UnitTests.h"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

#include "Print.h"
#include "Utils.h"

// TODO
// Clean up since this is getting larger

namespace
{
    ///////////////////////////////////////////////////////////////////////////
    // Test "framework"
    ///////////////////////////////////////////////////////////////////////////

    // TODO
    // Make into some sort of terminal output library to control colors, positions, etc.

    /**
     * Formats a colored string for terminal output
     * 
     * @param arBuffer Buffer to output to
     * @param apString The string to output
     * @param aColor The color the string should be (terminal escape sequence color)
     */
    template<std::size_t BufferSize>
    void FormatColoredString(char (&arBuffer)[BufferSize], const char* const apString, const uint32_t aColor)
    {
        Print::FormatToBuffer(arBuffer, "\x1b[{}m{}\x1b[m", aColor, apString);
    }

    /**
     * Tiny helper to emit pass or failure based on a result bool
     * 
     * @param aResult The result of the test
     * @param apMessage The message to emit
     */
    void EmitTestResult(const bool aResult, const char* const apMessage)
    {
        char passFailMessage[32];
        if (aResult)
        {
            FormatColoredString(passFailMessage, "PASS", 32 /*green*/);
        }
        else
        {
            FormatColoredString(passFailMessage, "FAIL", 31 /*red*/);
        }
        Print::FormatToMiniUART("[{}] {}\r\n", passFailMessage, apMessage);
    }

    ///////////////////////////////////////////////////////////////////////////
    // cstddef/stddef.h tests
    ///////////////////////////////////////////////////////////////////////////
    struct TestOffsetStruct
    {
        uint8_t ExpectedAt0;
        uint32_t ExpectedAt4;
        uint64_t ExpectedAt8;
    };

    static_assert(sizeof(std::size_t) == 8, "Unexpected size_t size");
    static_assert(sizeof(std::ptrdiff_t) == 8, "Unexpected ptrdiff_t size");
    static_assert(NULL == 0, "Unexpected NULL value");
    static_assert(alignof(std::max_align_t) == 16, "Unexpected max_align_t alignment");
    static_assert(offsetof(TestOffsetStruct, ExpectedAt0) == 0, "Unexpected offsetof first member");
    static_assert(offsetof(TestOffsetStruct, ExpectedAt4) == 4, "Unexpected offsetof second member");
    static_assert(offsetof(TestOffsetStruct, ExpectedAt8) == 8, "Unexpected offsetof third member");

    ///////////////////////////////////////////////////////////////////////////
    // cstdint/stdint.h tests
    ///////////////////////////////////////////////////////////////////////////
    static_assert(sizeof(std::int8_t) == 1, "Unexpected int8_t size");
    static_assert(sizeof(std::int16_t) == 2, "Unexpected int16_t size");
    static_assert(sizeof(std::int32_t) == 4, "Unexpected int32_t size");
    static_assert(sizeof(std::int64_t) == 8, "Unexpected int64_t size");

    static_assert(sizeof(std::intptr_t) == sizeof(void*), "Unexpected intptr_t size");

    static_assert(sizeof(std::uint8_t) == 1, "Unexpected uint8_t size");
    static_assert(sizeof(std::uint16_t) == 2, "Unexpected uint16_t size");
    static_assert(sizeof(std::uint32_t) == 4, "Unexpected uint32_t size");
    static_assert(sizeof(std::uint64_t) == 8, "Unexpected uint64_t size");

    static_assert(sizeof(std::uintptr_t) == sizeof(void*), "Unexpected uintptr_t size");

    static_assert(INT8_MIN == -128, "Unexpected int8 minimum");
    static_assert(INT16_MIN == -32768, "Unexpected int16 minimum");
    static_assert(INT32_MIN == -2147483648, "Unexpected int32 minimum");
    static_assert(INT64_MIN == -9223372036854775807LL - 1LL, "Unexpected int64 minimum"); // see note in stdint.h

    static_assert(INTPTR_MIN == -9223372036854775807LL - 1LL, "Unexpected intptr minimum"); // see note in stdint.h

    static_assert(INT8_MAX == 127, "Unexpected int8 maximum");
    static_assert(INT16_MAX == 32767, "Unexpected int16 maximum");
    static_assert(INT32_MAX == 2147483647, "Unexpected int32 maximum");
    static_assert(INT64_MAX == 9223372036854775807, "Unexpected int64 maximum");

    static_assert(INTPTR_MAX == 9223372036854775807, "Unexpected intptr maximum");

    static_assert(UINT8_MAX == 255, "Unexpected uint8 maximum");
    static_assert(UINT16_MAX == 65535, "Unexpected uint16 maximum");
    static_assert(UINT32_MAX == 4294967295, "Unexpected uint32 maximum");
    static_assert(UINT64_MAX == 18446744073709551615ULL, "Unexpected uint64 maximum");

    static_assert(UINT64_MAX == 18446744073709551615ULL, "Unexpected uintptr maximum");

    ///////////////////////////////////////////////////////////////////////////
    // cstring tests
    ///////////////////////////////////////////////////////////////////////////

    void StrcmpEqualTest()
    {
        EmitTestResult(strcmp("Hello", "Hello") == 0, "strcmp equality");
    }

    void StrcmpLTTest()
    {
        EmitTestResult(strcmp("ABC", "BCD") < 0, "strcmp less than");
        EmitTestResult(strcmp("ABC", "ACD") < 0, "strcmp less than - partial");
        EmitTestResult(strcmp("ABC", "ABCD") < 0, "strcmp less than - differing lengths");
    }

    void StrcmpGTTest()
    {
        EmitTestResult(strcmp("BCD", "ABC") > 0, "strcmp greater than");
        EmitTestResult(strcmp("ACD", "ABC") > 0, "strcmp greater than - partial");
        EmitTestResult(strcmp("ABCD", "ABC") > 0, "strcmp greater than - differing lengths");
    }

    ///////////////////////////////////////////////////////////////////////////
    // type_traits tests
    ///////////////////////////////////////////////////////////////////////////

    struct DummyStruct
    {
        int MemberFunction() const;
    };
    int DummyFunction();

    template<typename>
    struct PointerMemberTraits {};
    template<class T, class U>
    struct PointerMemberTraits<U T::*>
    {
        using member_type = U;
    };

    static_assert(std::is_same_v<std::integral_constant<uint32_t, 10u>::value_type, uint32_t>, "Unexpected integral_constant::value_type");
    static_assert(std::is_same_v<std::integral_constant<uint32_t, 10u>::type, std::integral_constant<uint32_t, 10u>>, "Unexpected integral_constant::type");
    static_assert(std::integral_constant<uint32_t, 10u>::value == 10u, "Unexpected integral_constant::value");
    static_assert(static_cast<uint32_t>(std::integral_constant<uint32_t, 10u>{}) == 10u, "Unexpected integral_constant value via cast operation");
    static_assert(std::integral_constant<uint32_t, 10u>{}() == 10u, "Unexpected integral_constant value via invoke operation");

    // these test bool_constant
    static_assert(std::is_same_v<std::true_type::value_type, bool>, "Unexpected true_type::value_type");
    static_assert(std::true_type::value == true, "Unexpected true_type::value");
    static_assert(std::is_same_v<std::false_type::value_type, bool>, "Unexpected true_type::value_type");
    static_assert(std::false_type::value == false, "Unexpected false_type::value");

    static_assert(std::is_same_v<uint32_t, uint32_t>, "Unexpected result for is_same with same types");
    static_assert(!std::is_same_v<uint32_t, int32_t>, "Unexpected result for is_same with different types");

    static_assert(std::is_const_v<const int>, "Unexpected result for is_const with const type");
    static_assert(!std::is_const_v<int>, "Unexpected result for is_const with non-const type");
    static_assert(!std::is_const_v<const int*>, "Unexpected result for is_const with non-const type (with non-top-level const)");

    static_assert(std::is_reference_v<int&>, "Unexpected result for is_reference with lvalue reference type");
    static_assert(std::is_reference_v<int&&>, "Unexpected result for is_reference with rvaule reference type");
    static_assert(!std::is_reference_v<int>, "Unexpected result for is_reference with non-reference type");

    static_assert(std::is_lvalue_reference_v<int&>, "Unexpected result for is_lvalue_reference with lvalue reference type");
    static_assert(!std::is_lvalue_reference_v<int&&>, "Unexpected result for is_lvalue_reference with rvaule reference type");
    static_assert(!std::is_lvalue_reference_v<int>, "Unexpected result for is_lvalue_reference with non-reference type");

    static_assert(std::is_array_v<int[]>, "Unexpected result for is_array with array type");
    static_assert(std::is_array_v<int[5]>, "Unexpected result for is_array with bounded array type");
    static_assert(!std::is_array_v<int*>, "Unexpected result for is_array with non-array type");

    static_assert(std::is_function_v<decltype(DummyFunction)>, "Unexpected result for is_function with function");
    static_assert(std::is_function_v<int(int)>, "Unexpected result for is_function with function");
    static_assert(std::is_function_v<PointerMemberTraits<decltype(&DummyStruct::MemberFunction)>::member_type>, "Unexpected result for is_function with member function");
    static_assert(!std::is_function_v<DummyStruct>, "Unexpected result for is_function with struct");
    static_assert(!std::is_function_v<int>, "Unexpected result for is_function with base type");
    static_assert(!std::is_function_v<int&>, "Unexpected result for is_function with reference");
    static_assert(!std::is_function_v<int(*)(int)>, "Unexpected result for is_function with pointer to function");

    static_assert(std::is_same_v<std::remove_reference_t<int*>, int*>, "Unexpected result from remove_reference with non-reference type");
    static_assert(std::is_same_v<std::remove_reference_t<int&>, int>, "Unexpected result from remove_reference with lvalue reference type");
    static_assert(std::is_same_v<std::remove_reference_t<int&&>, int>, "Unexpected result from remove_reference with rvalue reference type");

    static_assert(std::is_same_v<std::remove_extent_t<int*>, int*>, "Unexpected result from remove_extent with non-array type");
    static_assert(std::is_same_v<std::remove_extent_t<int[]>, int>, "Unexpected result from remove_extent with array type");
    static_assert(std::is_same_v<std::remove_extent_t<int[10]>, int>, "Unexpected result from remove_extent with sized array type");

    static_assert(std::is_same_v<std::remove_cv_t<int>, int>, "Unexpected result from remove_cv with non-const, non-volatile type");
    static_assert(std::is_same_v<std::remove_cv_t<const int>, int>, "Unexpected result from remove_cv with const, non-volatile type");
    static_assert(std::is_same_v<std::remove_cv_t<volatile int>, int>, "Unexpected result from remove_cv with non-const, volatile type");
    static_assert(std::is_same_v<std::remove_cv_t<const volatile int>, int>, "Unexpected result from remove_cv with const, volatile type");

    static_assert(std::is_same_v<std::add_pointer_t<int>, int*>, "Unexpected result from add_pointer with base type");
    static_assert(std::is_same_v<std::add_pointer_t<int*>, int**>, "Unexpected result from add_pointer with pointer type");
    static_assert(std::is_same_v<std::add_pointer_t<int&>, int*>, "Unexpected result from add_pointer with reference type");
    static_assert(std::is_same_v<std::add_pointer_t<int(int)>, int(*)(int)>, "Unexpected result from add_pointer with function type");
    static_assert(std::is_same_v<std::add_pointer_t<int(int) const>, int(int) const>, "Unexpected result from add_pointer with cv-qualified function type");

    static_assert(std::is_same_v<std::conditional_t<true, int32_t, int8_t>, int32_t>, "Unexpected result from true conditional");
    static_assert(std::is_same_v<std::conditional_t<false, int32_t, int8_t>, int8_t>, "Unexpected result from false conditional");

    static_assert(std::is_same_v<std::decay_t<int>, int>, "Unexpected result from std::decay for base type");
    static_assert(std::is_same_v<std::decay_t<int&>, int>, "Unexpected result from std::decay for lvalue ref type");
    static_assert(std::is_same_v<std::decay_t<int&&>, int>, "Unexpected result from std::decay for rvalue ref type");
    static_assert(std::is_same_v<std::decay_t<const int&>, int>, "Unexpected result from std::decay for const lvalue ref type");
    static_assert(std::is_same_v<std::decay_t<int[2]>, int*>, "Unexpected result from std::decay for array type");
    static_assert(std::is_same_v<std::decay_t<int(int)>, int(*)(int)>, "Unexpected result from std::decay for function type");

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
        Print::FormatToBuffer(buffer, "Test {}, test {}", 1u, 102u);
        EmitTestResult(strcmp(buffer, "Test 1, test 102") == 0, "Print::FormatToBuffer with integer arguments");
    }

    /**
     * Ensure printing to a buffer with integer arguments and a too-small buffer works
     */
    void PrintIntegerArgsTruncatedBufferTest()
    {
        char buffer[15];
        Print::FormatToBuffer(buffer, "Test {}, test {}", 1u, 102u);
        EmitTestResult(strcmp(buffer, "Test 1, test 1") == 0, "Print::FormatToBuffer with integer arguments");
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
        EmitTestResult(CPU::GetExceptionLevel() == 1, "Exception level");
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
            : "memory", "v0", "v1" // clobbered registers
        );

        auto success = true;
        for (int i = 0; (i < 4) && success; ++i)
        {
            success = (results[i] == (leftValues[i] + rightValues[i]));
        }

        EmitTestResult(success, "SIMD Instructions");
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

        StdMoveTest();
        StdForwardTest();
        StrcmpEqualTest();
        StrcmpLTTest();
        StrcmpGTTest();

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
    }

    void RunPostStaticDestructors()
    {
        StaticCDestructorTest();
        StaticCppDestructorTest();
    }
}