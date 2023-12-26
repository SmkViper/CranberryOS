#include "Framework.h"

#include <cstdint>

#include "../Print.h"

namespace UnitTests
{
    namespace
    {
        /**
         * Formats a colored string for terminal output
         * 
         * @param arBuffer Buffer to output to
         * @param apString The string to output
         * @param aColor The color the string should be (terminal escape sequence color)
         */
        template<std::size_t BufferSize>
        void FormatColoredString(char (&arBuffer)[BufferSize], char const* const apString, uint32_t const aColor)
        {
            Print::FormatToBuffer(arBuffer, "\x1b[{}m{}\x1b[m", aColor, apString);
        }
    }

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

    void EmitTestSkipResult(char const* const apMessage)
    {
        char skipMessage[32];
        FormatColoredString(skipMessage, "SKIP", 33 /*yellow*/);
        Print::FormatToMiniUART("[{}] {}\r\n", skipMessage, apMessage);
    }
}