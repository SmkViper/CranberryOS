#include "Print.h"

#include "MiniUart.h"

namespace
{
    struct NumericOutputFormat
    {
        uint8_t Base = 10u;
        bool IsUppercase = false;
    };

    /**
     * Converts a numeric output character to the format information
     * 
     * @param aFormat The char to convert
     * 
     * @return The output format to use
     */
    NumericOutputFormat ConvertOutputFormat(const char aFormat)
    {
        auto retVal = NumericOutputFormat{};
        switch (aFormat)
        {
        case 'B':
            retVal.IsUppercase = true;
            [[fallthrough]];
        case 'b':
            retVal.Base = 2u;
            break;
                
        case 'o':
            retVal.Base = 8u;
            break;

        case 'X':
            retVal.IsUppercase = true;
            [[fallthrough]];
        case 'x':
            retVal.Base = 16u;
            break;

        case 'd':
            break;

        default:
            // TODO
            // throw exception - invalid
            break;
        }
        return retVal;
    }

    /**
     * Converts a digit to a numer or letter, optionally uppercase
     * 
     * @param aDigit The digit to convert
     * @param aUppercase Should the character be uppercase?
     * 
     * @return The character representing the digit
     */
    char DigitToChar(const uint8_t aDigit, const bool aUppercase)
    {
        auto retVal = '0' + aDigit;
        if (aDigit >= 10)
        {
            const auto letterBase = aUppercase ? 'A' : 'a';
            retVal = letterBase + (aDigit - 10);
        }
        return retVal;
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
    bool OutputIntegerPrefix(const char aFormat, const bool aIsZero, Print::Detail::OutputFunctorBase& arOutput)
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

        case 'd':
            // decimal has no prefix
            break;

        default:
            // TODO
            // throw exception - invalid
            break;
        }

        return success;
    }

    /**
     * Output an integer to the specified output
     * 
     * @param aValue The value to output
     * @param aFormat The format character for the number
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool OutputInteger(const uint64_t aValue, const char aFormat, Print::Detail::OutputFunctorBase& arOutput)
    {
        // TODO
        // Handle 'c' format

        auto success = true;
        if (aValue == 0)
        {
            success = OutputIntegerPrefix(aFormat, true, arOutput);
            success = success && arOutput.WriteChar('0');
        }
        else
        {
            // TODO
            // We'll need to make this more generic with various integer sizes, signed/unsigned, bases, etc

            success = OutputIntegerPrefix(aFormat, false, arOutput);
            const auto outputFormat = ConvertOutputFormat(aFormat);

            // Stores each base-X digit, from least to most significant, followed by the prefix, if needed
            static constexpr auto maxDigits = 64; // enough bits for a 64 bit value in any base
            uint8_t digits[maxDigits] = {0};
            auto numberOfDigits = 0u;

            auto remainingValue = aValue;
            while ((remainingValue > 0) && (numberOfDigits < maxDigits))
            {
                digits[numberOfDigits] = remainingValue % outputFormat.Base;
                remainingValue /= outputFormat.Base;
                ++numberOfDigits;
            }

            for (auto curDigit = 0u; (curDigit < numberOfDigits) && success; ++curDigit)
            {
                success = arOutput.WriteChar(DigitToChar(digits[numberOfDigits - curDigit - 1], outputFormat.IsUppercase));
            }
        }
        return success;
    }
}

namespace Print
{
    namespace Detail
    {
        /**
         * Write out a single character to the output - overridden by implementation
         * 
         * @param aChar The character to write
         * 
         * @return True on success
         */
        bool MiniUARTOutputFunctor::WriteCharImpl(const char aChar)
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
        bool DataWrapper<uint32_t>::OutputDataImpl(const char aFormat, OutputFunctorBase& arOutput) const
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
        bool DataWrapper<uint64_t>::OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const
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
        bool DataWrapper<const char*>::OutputDataImpl(char /*aFormat*/, OutputFunctorBase& arOutput) const
        {
            auto success = true;
            if (pWrappedData != nullptr) // we'll output null strings as nothing, "successfully"
            {
                auto pcurChar = pWrappedData;
                while (success && (*pcurChar != '\0'))
                {
                    success = arOutput.WriteChar(*pcurChar);
                    ++pcurChar;
                }
            }
            return success;
        }

        /**
         * Formats the string and sends it to the given output
         * 
         * @param apFormatString The format string, following the std::format syntax
         * @param arOutput The output to send to
         * @param apDataArray First element in an array of elements to substitute into the format string
         * @param aDataCount Number of items in the data array
         */
        void FormatImpl(const char* const apFormatString, OutputFunctorBase& arOutput, const DataWrapperBase** const apDataArray, const std::size_t aDataCount)
        {
            enum class ParseState
            {
                OutputCharacter,
                OpenBrace,
                CloseBrace,
                EscapedCloseBrace,
                FormatString,
            };

            if (apFormatString)
            {
                auto success = true;
                auto pcurChar = apFormatString;
                auto curState = ParseState::OutputCharacter;
                auto curFormat = 'd';
                auto curDataElement = 0u;

                auto outputElement = [aDataCount, apDataArray, &arOutput](const uint32_t aElement, const char aFormat)
                {
                    auto success = true;
                    if (aElement < aDataCount)
                    {
                        // no need to null check the contents of apDataArray, since it's generated by functions
                        // that fill it with pointers to stack values
                        success = apDataArray[aElement]->OutputData(aFormat, arOutput);
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
                            // TODO
                            // throw exception - invalid
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
                            // TODO
                            // throw exception - invalid
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
                            // TODO
                            // throw exception - invalid
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
                            // TODO
                            // support full formatting options
                            curFormat = *pcurChar;
                            curState = ParseState::CloseBrace;
                            writeChar = false;
                        }
                        break;

                    default:
                        // TODO
                        // assert - unexpected state
                        break;
                    }

                    if (writeChar)
                    {
                        success = arOutput.WriteChar(*pcurChar);
                    }
                    ++pcurChar;
                }
            }
        }
    }
}