#include "CStringTests.h"

#include <cstdint>
#include <cstring>

#include "../Framework.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
namespace UnitTests::KernelStdlib::CString
{
    namespace
    {
        /**
         * Run tests on memcpy
         */
        void MemcpyTest()
        {
            constexpr unsigned arraySize = 4;
            constexpr unsigned elementsToCopy = 2;

            // we're doing partial memcpy to ensure it doesn't bash anything past what it's told to write
            char charArraySrc[arraySize] = {'a', 'b', 'c', 'd'};
            char charArrayDst[arraySize] = {};
            static_assert(sizeof(charArraySrc) == sizeof(charArrayDst), "Size mismatch");

            EmitTestResult(memcpy(charArrayDst, charArraySrc, elementsToCopy * sizeof(char)) == charArrayDst, "memcpy char array return value");
            EmitTestResult((charArrayDst[0] == 'a') && (charArrayDst[1] == 'b') && (charArrayDst[2] == 0) && (charArrayDst[3] == 0), "memcpy copies partial char array");

            uint32_t intArraySrc[arraySize] = {10U, 15U, 20U, 25U};
            uint32_t intArrayDst[arraySize] = {};
            static_assert(sizeof(intArraySrc) == sizeof(intArrayDst), "Size mismatch");

            EmitTestResult(memcpy(intArrayDst, intArraySrc, elementsToCopy * sizeof(uint32_t)) == intArrayDst, "memcpy int array return value");
            EmitTestResult((intArrayDst[0] == 10U) && (intArrayDst[1] == 15U) && (intArrayDst[2] == 0) && (intArrayDst[3] == 0), "memcpy copies partial int array");
        }

        /**
         * Run tests on memset
         */
        void MemsetTest()
        {
            constexpr unsigned arraySize = 4;
            constexpr unsigned elementsToCopy = 2;

            // we're doing partial memset to ensure it doesn't bash anything past what it's told to write
            char charArray[arraySize] = {'a', 'b', 'c', 'd'};
            EmitTestResult(memset(charArray, 0U, elementsToCopy * sizeof(char)) == charArray, "memset char array return value");
            EmitTestResult((charArray[0] == 0U) && (charArray[1] == 0U) && (charArray[2] == 'c') && (charArray[3] == 'd'), "memset fills partial char array");

            uint32_t intArray[arraySize] = {10U, 15U, 20U, 25U};
            EmitTestResult(memset(intArray, 1U, elementsToCopy * sizeof(uint32_t)) == intArray, "memset int array return value");
            constexpr auto expectedInt = (1U << 24U) | (1U << 16U) | (1U << 8U) | 1U;
            EmitTestResult((intArray[0] == expectedInt) && (intArray[1] == expectedInt) && (intArray[2] == 20U) && (intArray[3] == 25U), "memset fills partial int array");
        }

        /**
         * Make sure strcmp handles equality
         */
        void StrcmpEqualTest()
        {
            EmitTestResult(strcmp("Hello", "Hello") == 0, "strcmp equality");
        }

        /**
         * Make sure strcmp handles less than
         */
        void StrcmpLTTest()
        {
            EmitTestResult(strcmp("ABC", "BCD") < 0, "strcmp less than");
            EmitTestResult(strcmp("ABC", "ACD") < 0, "strcmp less than - partial");
            EmitTestResult(strcmp("ABC", "ABCD") < 0, "strcmp less than - differing lengths");
        }

        /**
         * Make sure strcmp handles greater than
         */
        void StrcmpGTTest()
        {
            EmitTestResult(strcmp("BCD", "ABC") > 0, "strcmp greater than");
            EmitTestResult(strcmp("ACD", "ABC") > 0, "strcmp greater than - partial");
            EmitTestResult(strcmp("ABCD", "ABC") > 0, "strcmp greater than - differing lengths");
        }

        // This clang tidy checker seems to want the third paramter of strncmp to be equal to or less than the length
        // of one of the strings, even though strncmp can explicitly handle more (probably to catch people using
        // strncmp when they could use a cheaper strcmp). But we want to make sure our implementation of strncmp works
        // under these conditions, so disable the warning
        // NOLINTBEGIN(bugprone-not-null-terminated-result)

        /**
         * Make sure strncmp handles equality
         */
        void StrncmpEqualTest()
        {
            EmitTestResult(strncmp("Hello", "Hello", 10) == 0, "strncmp equality, full string");
            EmitTestResult(strncmp("Hello", "Heppy", 2) == 0, "strncmp equality, substring");
        }

        /**
         * Make sure strncmp handles less than
         */
        void StrncmpLTTest()
        {
            EmitTestResult(strncmp("ABC", "BCD", 10) < 0, "strncmp less than, full string");
            EmitTestResult(strncmp("ABC", "ACD", 10) < 0, "strncmp less than - partial, full string");
            EmitTestResult(strncmp("ABC", "ABCD", 10) < 0, "strncmp less than - differing lengths, full string");
            EmitTestResult(strncmp("ABC", "BCD", 2) < 0, "strncmp less than, substring");
        }

        /**
         * Make sure strncmp handles greater than
         */
        void StrncmpGTTest()
        {
            EmitTestResult(strncmp("BCD", "ABC", 10) > 0, "strncmp greater than, full string");
            EmitTestResult(strncmp("ACD", "ABC", 10) > 0, "strncmp greater than - partial, full string");
            EmitTestResult(strncmp("ABCD", "ABC", 10) > 0, "strncmp greater than - differing lengths, full string");
            EmitTestResult(strncmp("ACD", "ABC", 2) > 0, "strncmp greater than, substring");
        }

        // NOLINTEND(bugprone-not-null-terminated-result)

        /**
         * Run tests on strlen
         */
        void StrlenTest()
        {
            EmitTestResult(strlen("") == 0, "strlen empty string");
            EmitTestResult(strlen("Hello") == 5, "strlen non-empty string");
            EmitTestResult(strlen("Hel\0lo") == 3, "strlen embedded null");
        }
    }

    void Run()
    {
        MemcpyTest();
        MemsetTest();

        StrcmpEqualTest();
        StrcmpLTTest();
        StrcmpGTTest();

        StrncmpEqualTest();
        StrncmpLTTest();
        StrncmpGTTest();
        
        StrlenTest();
    }
}
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
// NOLINTEND(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)