#include "CStringTests.h"

#include <cstdint>
#include <cstring>

#include "../Framework.h"

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

            uint32_t intArraySrc[arraySize] = {10u, 15u, 20u, 25u};
            uint32_t intArrayDst[arraySize] = {};
            static_assert(sizeof(intArraySrc) == sizeof(intArrayDst), "Size mismatch");

            EmitTestResult(memcpy(intArrayDst, intArraySrc, elementsToCopy * sizeof(uint32_t)) == intArrayDst, "memcpy int array return value");
            EmitTestResult((intArrayDst[0] == 10u) && (intArrayDst[1] == 15u) && (intArrayDst[2] == 0) && (intArrayDst[3] == 0), "memcpy copies partial int array");
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
            EmitTestResult(memset(charArray, 0u, elementsToCopy * sizeof(char)) == charArray, "memset char array return value");
            EmitTestResult((charArray[0] == 0u) && (charArray[1] == 0u) && (charArray[2] == 'c') && (charArray[3] == 'd'), "memset fills partial char array");

            uint32_t intArray[arraySize] = {10u, 15u, 20u, 25u};
            EmitTestResult(memset(intArray, 1u, elementsToCopy * sizeof(uint32_t)) == intArray, "memset int array return value");
            constexpr auto expectedInt = (1u << 24u) | (1u << 16u) | (1u << 8u) | 1u;
            EmitTestResult((intArray[0] == expectedInt) && (intArray[1] == expectedInt) && (intArray[2] == 20u) && (intArray[3] == 25u), "memset fills partial int array");
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
        StrlenTest();
    }
}