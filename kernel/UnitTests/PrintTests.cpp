#include "PrintTests.h"

#include <cstring>
#include "Framework.h"
#include "../Print.h"

namespace UnitTests::Print
{
    namespace
    {
        class MockOutputFunctor: public ::Print::Detail::OutputFunctorBase
        {
        public:
            char LastWriteCharParam = '\0';
            bool WriteCharRetVal = true;
        private:
            bool WriteCharImpl(char const aChar) override
            {
                LastWriteCharParam = aChar;
                return WriteCharRetVal;
            }
        };

        /**
         * Ensure Detail::OutputFunctorBase::WriteChar() forwards parameters and return values correctly
         */
        void OutputFunctorBaseWriteCharForwardsData()
        {
            MockOutputFunctor functor;

            const auto result1 = functor.WriteChar('a');
            EmitTestResult(result1 && (functor.LastWriteCharParam == 'a'), "OutputFunctorBase::WriteChar forwards data");
            
            functor.WriteCharRetVal = false;
            const auto result2 = functor.WriteChar('b');
            EmitTestResult(!result2 && (functor.LastWriteCharParam == 'b'), "OutputFunctorBas::WriteChar handles failure");
        }

        // MiniUARTOutputFunctor not really testable as it outputs to hardware. Though if it fails it should be obvious
        // because we use it a lot

        /**
         * Ensure Detail::StaticBufferOutputFunctor::WriteChar() writes characters, updates class data, and handles out
         * of buffer
        */
        void StaticBufferOutputFunctorWritesChars()
        {
            char buffer[2] = {};
            ::Print::Detail::StaticBufferOutputFunctor functor{ buffer };

            const auto result1 = functor.WriteChar('a');
            EmitTestResult(result1 && (functor.GetCharsWritten() == 1u) && (buffer[0] == 'a'), "StaticBufferOutputFunctor::WriteChar outputs character");
            const auto result2 = functor.WriteChar('b');
            EmitTestResult(result2 && (functor.GetCharsWritten() == 2u) && (buffer[1] == 'b'), "StaticBufferOutputFunctor::WriteChar outputs second character");
            const auto result3 = functor.WriteChar('c');
            EmitTestResult(!result3 && (functor.GetCharsWritten() == 2u), "StaticBufferOutputFunctor::WriteChar returns failure if off buffer end");
        }

        class MockDataWrapper: public ::Print::Detail::DataWrapperBase
        {
        public:
            char mutable LastFormatChar = '\0';
            bool OutputDataRetVal = true;
            ::Print::Detail::OutputFunctorBase mutable* pLastOutputFunctor = nullptr;
        private:
            bool OutputDataImpl(char const aFormat, ::Print::Detail::OutputFunctorBase& arOutput) const override
            {
                LastFormatChar = aFormat;
                pLastOutputFunctor = &arOutput;
                return OutputDataRetVal;
            }
        };

        /**
         * Ensure Detail::DataWrapperBase::OutputData() forwards parameters and return values correctly
         */
        void DataWrapperBaseOutputDataForwardsData()
        {
            MockDataWrapper functor;
            MockOutputFunctor outputFunctor;

            const auto result1 = functor.OutputData('a', outputFunctor);
            EmitTestResult(result1 && (functor.LastFormatChar == 'a') && (functor.pLastOutputFunctor == &outputFunctor), "DataWrapperBase::OutputData forwards data");
            
            functor.OutputDataRetVal = false;
            const auto result2 = functor.OutputData('b', outputFunctor);
            EmitTestResult(!result2 && (functor.LastFormatChar == 'b') && (functor.pLastOutputFunctor == &outputFunctor), "DataWrapperBase::OutputData handles failure");
        }

        // DataWrapper<T> static_asserts if ever instantiated, so we can't test that

        /**
         * Helper for testing integer data wrapper OutputData()
         * 
         * @tparam T The integer type to test with
         * @tparam BufferSize the size of the buffer to write to
         * @param aValue The value to format
         * @param aFormat The format to output
         * @param aExpectedResult The expected result from the OutputData call
         * @param apExpectedOutput The expected string output from the OutputData call
         * @return True if the tests pass
        */
        template<typename T, size_t BufferSize>
        bool OutputDataTestHelper(T const aValue, char const aFormat, bool const aExpectedResult, char const* apExpectedOutput)
        {
            ::Print::Detail::DataWrapper<T> wrapper{ aValue };
            char buffer[BufferSize];
            ::Print::Detail::StaticBufferOutputFunctor bufferFunctor{ buffer };

            auto const result = wrapper.OutputData(aFormat, bufferFunctor);

            // #TODO: Replace with strncmp when we implement it
            char compareBuffer[BufferSize + 1] = {};
            memcpy(compareBuffer, buffer, BufferSize * sizeof(char));
            compareBuffer[bufferFunctor.GetCharsWritten()] = '\0';

            auto const outputMatches = (strcmp(compareBuffer, apExpectedOutput) == 0);

            return (result == aExpectedResult) && outputMatches;
        }

        /**
         * Tests for DataWrapper::OutputData
         */
        void DataWrapperOutputDataTest()
        {
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0xFE, 'd', true, "254"), "DataWrapper<uint8_t>::OutputData decimal");
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0xFE, 'o', true, "0376"), "DataWrapper<uint8_t>::OutputData octal");
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0, 'o', true, "0"), "DataWrapper<uint8_t>::OutputData octal zero");
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0xFE, 'x', true, "0xfe"), "DataWrapper<uint8_t>::OutputData hex lower");
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0xFE, 'X', true, "0XFE"), "DataWrapper<uint8_t>::OutputData hex upper");
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0xFE, 'b', true, "0b11111110"), "DataWrapper<uint8_t>::OutputData binary lower");
            EmitTestResult(OutputDataTestHelper<uint8_t, 64>(0xFE, 'B', true, "0B11111110"), "DataWrapper<uint8_t>::OutputData binary upper");
            EmitTestResult(OutputDataTestHelper<uint8_t, 2>(0xFE, 'd', false, "25"), "DataWrapper<uint8_t>::OutputData out of space in number");
            EmitTestResult(OutputDataTestHelper<uint8_t, 1>(0xFE, 'x', false, "0"), "DataWrapper<uint8_t>::OutputData out of space in header");
            // Aren't going to re-run all of the above for each type we support, since the internal guts are identical
            EmitTestResult(OutputDataTestHelper<uint16_t, 64>(0xFEDC, 'X', true, "0XFEDC"), "DataWrapper<uint16_t>::OutputData hex upper");
            EmitTestResult(OutputDataTestHelper<uint32_t, 64>(0xFEDC'9876, 'X', true, "0XFEDC9876"), "DataWrapper<uint32_t>::OutputData hex upper");
            EmitTestResult(OutputDataTestHelper<uint64_t, 64>(0xFEDC'9876'BA98'5432ull, 'X', true, "0XFEDC9876BA985432"), "DataWrapper<uint64_t>::OutputData hex upper");
            EmitTestResult(OutputDataTestHelper<size_t, 64>(0xFEDC'9876'BA98'5432ul, 'X', true, "0XFEDC9876BA985432"), "DataWrapper<size_t>::OutputData hex upper");

            EmitTestResult(OutputDataTestHelper<char const*, 64>("Hello", '\0', true, "Hello"), "DataWrapper<char const*>::OutputData");
            EmitTestResult(OutputDataTestHelper<char const*, 3>("Hello", '\0', false, "Hel"), "DataWrapper<char const*>::OutputData out of space");

            char nonConstBuffer[10] = {'H', 'i', '\0'};
            EmitTestResult(OutputDataTestHelper<char*, 64>(nonConstBuffer, '\0', true, "Hi"), "DataWrapper<char*>::OutputData");
        }

        // FormatToMiniUART goes to hardware, so not able to test directly. We'll see if any issues crop up easily
        // enough, plus it shares all its code (minus the output functor) with the buffer print, so those tests will
        // account for this function

        /**
         * Ensure printing to a buffer with no arguments works
         */
        void PrintNoArgsTest()
        {
            const char expectedOutput[] = "Hello World";
            char buffer[256];
            ::Print::FormatToBuffer(buffer, expectedOutput);

            EmitTestResult(strcmp(buffer, expectedOutput) == 0, "Print::FormatToBuffer with no args");
        }

        /**
         * Ensure printing to a buffer with no arguments and a too-small buffer works
         */
        void PrintNoArgsTruncatedBufferTest()
        {
            const char expectedOutput[] = "Hello World";
            char buffer[5];
            ::Print::FormatToBuffer(buffer, expectedOutput);

            EmitTestResult(strcmp(buffer, "Hell") == 0, "Print::FormatToBuffer with no args and a too-small buffer");
        }

        /**
         * Ensure printing to a buffer with string arguments (both raw string and pointer) works
         */
        void PrintStringArgsTest()
        {
            const char* pstringPointer = "Again";
            char buffer[256];
            ::Print::FormatToBuffer(buffer, "Hello {} {}", "World", pstringPointer);
            EmitTestResult(strcmp(buffer, "Hello World Again") == 0, "Print::FormatToBuffer with string arguments");
        }

        /**
         * Ensure printing to a buffer with string arguments (both raw string and pointer) and a too-small buffer works
         */
        void PrintStringArgsTruncatedBufferTest()
        {
            const char* pstringPointer = "Again";
            char buffer[8];
            ::Print::FormatToBuffer(buffer, "Hello {} {}", "World", pstringPointer);
            EmitTestResult(strcmp(buffer, "Hello W") == 0, "Print::FormatToBuffer with string arguments and a too-small buffer");
        }

        /**
         * Ensure printing to a buffer with integer arguments works
         */
        void PrintIntegerArgsTest()
        {
            char buffer[256];
            ::Print::FormatToBuffer(buffer, "Test {}, test {}, test {}", 1u, 102u, 0u);
            EmitTestResult(strcmp(buffer, "Test 1, test 102, test 0") == 0, "Print::FormatToBuffer with integer arguments");

            ::Print::FormatToBuffer(buffer, "Format Test {:}", 1u);
            EmitTestResult(strcmp(buffer, "Format Test 1") == 0, "Print::FormatToBuffer with integer arguments and empty format string");

            constexpr auto testBinaryNumber = 0b1100'1010u;
            ::Print::FormatToBuffer(buffer, "Binary Test {:b} {:B}", testBinaryNumber, testBinaryNumber);
            EmitTestResult(strcmp(buffer, "Binary Test 0b11001010 0B11001010") == 0, "Print::FormatToBuffer with integer arguments and binary format string");

            constexpr auto testOctalNumber = 0123u;
            ::Print::FormatToBuffer(buffer, "Octal Test {:o} {:o}", testOctalNumber, 0u);
            EmitTestResult(strcmp(buffer, "Octal Test 0123 0") == 0, "Print::FormatToBuffer with integer arguments and octal format string");

            constexpr auto testDecimalNumber = 123u;
            ::Print::FormatToBuffer(buffer, "Decimal Test {:d}", testDecimalNumber, testDecimalNumber);
            EmitTestResult(strcmp(buffer, "Decimal Test 123") == 0, "Print::FormatToBuffer with integer arguments and decimal format string");

            constexpr auto testHexNumber = 0x11ff89abu;
            ::Print::FormatToBuffer(buffer, "Hex Test {:x} {:X}", testHexNumber, testHexNumber);
            EmitTestResult(strcmp(buffer, "Hex Test 0x11ff89ab 0X11FF89AB") == 0, "Print::FormatToBuffer with integer arguments and hex format string");
        }

        /**
         * Ensure printing to a buffer with integer arguments and a too-small buffer works
         */
        void PrintIntegerArgsTruncatedBufferTest()
        {
            char buffer[15];
            ::Print::FormatToBuffer(buffer, "Test {}, test {}", 1u, 102u);
            EmitTestResult(strcmp(buffer, "Test 1, test 1") == 0, "Print::FormatToBuffer with integer arguments and a too-small buffer");
        }

        /**
         * Ensure escaped braces are printed
         */
        void PrintEscapedBracesTest()
        {
            char buffer[256];
            ::Print::FormatToBuffer(buffer, "Open {{ close }}");
            EmitTestResult(strcmp(buffer, "Open { close }") == 0, "Print::FormatToBuffer escaped braces");
        }

        /**
         * Ensure mismatched braces are handled
         */
        void PrintMismatchedBracesTest()
        {
            char buffer[256];
            ::Print::FormatToBuffer(buffer, "Close } some other text");
            EmitTestResult(strcmp(buffer, "Close ") == 0, "Print::FormatToBuffer mismatched close brace");

            ::Print::FormatToBuffer(buffer, "Open { some other text");
            EmitTestResult(strcmp(buffer, "Open ") == 0, "Print::FormatToBuffer mismatched open brace");
        }

        /**
         * Ensure invalid brace contents are handled
         */
        void PrintInvalidBraceContentsTest()
        {
            char buffer[256];
            ::Print::FormatToBuffer(buffer, "Hello {some bad text} world", "bad");
            EmitTestResult(strcmp(buffer, "Hello bad world") == 0, "Print::FormatToBuffer invalid brace contents");
        }

        /**
         * Ensure out of range braces are handled
         */
        void PrintOutOfRangeBracesTest()
        {
            char buffer[256];
            ::Print::FormatToBuffer(buffer, "Hello {} world {} again", "new");
            EmitTestResult(strcmp(buffer, "Hello new world {1} again") == 0, "Print::FormatToBuffer out of range braces");
        }
    }

    void Run()
    {
        OutputFunctorBaseWriteCharForwardsData();
        StaticBufferOutputFunctorWritesChars();

        DataWrapperBaseOutputDataForwardsData();
        DataWrapperOutputDataTest();

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
}