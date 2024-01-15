#include "Print.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "MiniUart.h"

namespace
{
    constexpr uint8_t BinaryBase = 2U;
    constexpr uint8_t DecimalBase = 10U;
    constexpr uint8_t OctalBase = 8U;
    constexpr uint8_t HexidecimalBase = 16U;

    struct NumericOutputFormat
    {
        uint8_t Base = DecimalBase;
        bool IsUppercase = false;
    };

    /**
     * Converts a numeric output character to the format information
     * 
     * @param aFormat The char to convert
     * 
     * @return The output format to use
     */
    NumericOutputFormat ConvertOutputFormat(char const aFormat)
    {
        auto retVal = NumericOutputFormat{};
        switch (aFormat)
        {
        case 'B':
            retVal.IsUppercase = true;
            [[fallthrough]];
        case 'b':
            retVal.Base = BinaryBase;
            break;
                
        case 'o':
            retVal.Base = OctalBase;
            break;

        case 'X':
            retVal.IsUppercase = true;
            [[fallthrough]];
        case 'x':
            retVal.Base = HexidecimalBase;
            break;

        // #TODO: Remove lint tag when we handle errors, since branches won't be identical anymore
        case 'd': // NOLINT(bugprone-branch-clone)
            break;

        default:
            // #TODO: throw exception - invalid
            break;
        }
        return retVal;
    }

    /**
     * Converts a digit to a number or letter, optionally uppercase
     * 
     * @param aDigit The digit to convert
     * @param aUppercase Should the character be uppercase?
     * 
     * @return The character representing the digit
     */
    char DigitToChar(uint8_t const aDigit, bool const aUppercase)
    {
        constexpr uint8_t decimalDigitMax = 10U;

        auto retVal = '0' + aDigit;
        if (aDigit >= decimalDigitMax)
        {
            const auto letterBase = aUppercase ? 'A' : 'a';
            retVal = letterBase + (aDigit - decimalDigitMax);
        }
        return static_cast<char>(retVal);
    }

    /**
     * Helper to output the appropriate integer prefix for the specified format
     * 
     * @param aFormat The format of integer being output
     * @param aIsZero Is the value being output zero?
     * @param arOutput The functor that outputs characters
     * 
     * @return True on success
     */
    bool OutputIntegerPrefix(char const aFormat, bool const aIsZero, Print::Detail::OutputFunctorBase& arOutput)
    {
        auto success = true;
        switch (aFormat)
        {
        case 'b':
        case 'B':
        case 'x':
        case 'X':
            success = arOutput.WriteChar('0');
            success = success && arOutput.WriteChar(aFormat);
            break;

        case 'o':
            // octal only outputs a leading zero if it is a non-zero value
            if (!aIsZero)
            {
                success = arOutput.WriteChar('0');
            }
            break;

        // #TODO: Remove lint tag when default branch emits errors
        case 'd': // NOLINT(bugprone-branch-clone)
            // decimal has no prefix
            break;

        default:
            // #TODO: throw exception - invalid
            break;
        }

        return success;
    }

    // #TODO: Need a better API that can't easily result in swappable parameters
    /**
     * Output an integer to the specified output
     * 
     * @param aValue The value to output
     * @param aFormat The format character for the number
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool OutputInteger(uint64_t const aValue, char const aFormat, Print::Detail::OutputFunctorBase& arOutput) // NOLINT(bugprone-easily-swappable-parameters)
    {
        // #TODO: Handle 'c' format

        auto success = true;
        if (aValue == 0)
        {
            success = OutputIntegerPrefix(aFormat, true, arOutput);
            success = success && arOutput.WriteChar('0');
        }
        else
        {
            // #TODO: We'll need to make this more generic with various integer sizes, signed/unsigned, bases, etc

            success = OutputIntegerPrefix(aFormat, false, arOutput);
            auto const outputFormat = ConvertOutputFormat(aFormat);

            // Stores each base-X digit, from least to most significant, followed by the prefix, if needed
            static constexpr auto maxDigits = 64; // enough bits for a 64 bit value in any base

            // #TODO: Remove lint tag when we have std::array
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            uint8_t digits[maxDigits] = {0};

            auto numberOfDigits = 0U;

            auto remainingValue = aValue;
            while ((remainingValue > 0) && (numberOfDigits < maxDigits))
            {
                // #TODO: Remove lint line when we get std::array
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                digits[numberOfDigits] = remainingValue % outputFormat.Base;
                remainingValue /= outputFormat.Base;
                ++numberOfDigits;
            }

            for (auto curDigit = 0U; (curDigit < numberOfDigits) && success; ++curDigit)
            {
                // #TODO: Remove lint line when we get std::array
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                success = arOutput.WriteChar(DigitToChar(digits[numberOfDigits - curDigit - 1], outputFormat.IsUppercase));
            }
        }
        return success;
    }
}

namespace Print::Detail
{
    /**
     * Write out a single character to the output - overridden by implementation
     * 
     * @param aChar The character to write
     * 
     * @return True on success
     */
    bool MiniUARTOutputFunctor::WriteCharImpl(char const aChar)
    {
        MiniUART::Send(aChar);
        return true;
    }

    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<uint8_t>::OutputDataImpl(char const aFormat, OutputFunctorBase& arOutput) const
    {
        return OutputInteger(WrappedData, aFormat, arOutput);
    }

    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<uint16_t>::OutputDataImpl(char const aFormat, OutputFunctorBase& arOutput) const
    {
        return OutputInteger(WrappedData, aFormat, arOutput);
    }

    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<uint32_t>::OutputDataImpl(char const aFormat, OutputFunctorBase& arOutput) const
    {
        return OutputInteger(WrappedData, aFormat, arOutput);
    }

    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<uint64_t>::OutputDataImpl(char const aFormat, OutputFunctorBase& arOutput) const
    {
        return OutputInteger(WrappedData, aFormat, arOutput);
    }

    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<std::size_t>::OutputDataImpl(char const aFormat, OutputFunctorBase& arOutput) const
    {
        return OutputInteger(WrappedData, aFormat, arOutput);
    }

    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<const char*>::OutputDataImpl(char const /*aFormat*/, OutputFunctorBase& arOutput) const
    {
        auto success = true;
        if (pWrappedData != nullptr) // we'll output null strings as nothing, "successfully"
        {
            auto const* pcurChar = pWrappedData;
            while (success && (*pcurChar != '\0'))
            {
                success = arOutput.WriteChar(*pcurChar);
                // #TODO: Can likely remove this when we have string_view available
                ++pcurChar; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
        }
        return success;
    }

    // #TODO: Need to simplify this function and remove the lint tag
    /**
     * Formats the string and sends it to the given output
     * 
     * @param apFormatString The format string, following the std::format syntax
     * @param arOutput The output to send to
     * @param apDataArray First element in an array of elements to substitute into the format string
     * @param aDataCount Number of items in the data array
     */
    void FormatImpl(char const* const apFormatString, OutputFunctorBase& arOutput, DataWrapperBase const* const* const apDataArray, std::size_t const aDataCount) // NOLINT(readability-function-cognitive-complexity)
    {
        enum class ParseState
        {
            OutputCharacter,
            OpenBrace,
            CloseBrace,
            EscapedCloseBrace,
            FormatString,
        };

        if (apFormatString != nullptr)
        {
            auto success = true;
            auto const* pcurChar = apFormatString;
            auto curState = ParseState::OutputCharacter;
            auto curFormat = 'd';
            auto curDataElement = 0U;

            auto outputElement = [aDataCount, apDataArray, &arOutput](uint32_t const aElement, char const aFormat)
            {
                auto success = true;
                if (aElement < aDataCount)
                {
                    // no need to null check the contents of apDataArray, since it's generated by functions
                    // that fill it with pointers to stack values
                    // #TODO: Remove lint tag when we get an array view of some kind
                    success = apDataArray[aElement]->OutputData(aFormat, arOutput); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
                else
                {
                    // TODO
                    // throw exception - invalid. For now, we output a placeholder
                    success = arOutput.WriteChar('{');
                    success = success && DataWrapper<std::remove_cv_t<decltype(aElement)>>{aElement}.OutputData('d', arOutput);
                    success = success && arOutput.WriteChar('}');
                }
                return success;
            };

            while (success && (*pcurChar != '\0'))
            {
                auto writeChar = true;
                switch (curState)
                {
                case ParseState::OutputCharacter:
                    if (*pcurChar == '{')
                    {
                        curState = ParseState::OpenBrace;
                        writeChar = false;
                    }
                    else if (*pcurChar == '}')
                    {
                        curState = ParseState::EscapedCloseBrace;
                        writeChar = false;
                    }
                    break;

                case ParseState::OpenBrace:
                    if (*pcurChar == '{')
                    {
                        // was an escaped open brace, so output the brace
                        curState = ParseState::OutputCharacter;
                    }
                    else if (*pcurChar == ':')
                    {
                        curState = ParseState::FormatString;
                        writeChar = false;
                    }
                    else if (*pcurChar == '}')
                    {
                        success = outputElement(curDataElement, 'd');
                        ++curDataElement;
                        curState = ParseState::OutputCharacter;
                        writeChar = false;
                    }
                    else
                    {
                        // #TODO: throw exception - invalid
                        writeChar = false;
                    }
                    break;

                case ParseState::CloseBrace:
                    if (*pcurChar == '}')
                    {
                        success = outputElement(curDataElement, curFormat);
                        ++curDataElement;
                        curFormat = 'd';
                        curState = ParseState::OutputCharacter;
                        writeChar = false;
                    }
                    else
                    {
                        // #TODO: throw exception - invalid
                        writeChar = false;
                    }
                    break;

                case ParseState::EscapedCloseBrace:
                    if (*pcurChar == '}')
                    {
                        // was an escaped close brace, so output the brace
                        curState = ParseState::OutputCharacter;
                    }
                    else
                    {
                        // #TODO: throw exception - invalid
                        writeChar = false;
                    }
                    break;

                case ParseState::FormatString:
                    if (*pcurChar == '}')
                    {
                        // empty format string
                        success = outputElement(curDataElement, 'd');
                        ++curDataElement;
                        curFormat = 'd';
                        curState = ParseState::OutputCharacter;
                        writeChar = false;
                    }
                    else
                    {
                        // #TODO: support full formatting options
                        curFormat = *pcurChar;
                        curState = ParseState::CloseBrace;
                        writeChar = false;
                    }
                    break;

                default:
                    // #TODO: assert - unexpected state
                    break;
                }

                if (writeChar)
                {
                    success = arOutput.WriteChar(*pcurChar);
                }
                // #TODO: Remove lint tag when we get string_view
                ++pcurChar; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
        }
    }
}