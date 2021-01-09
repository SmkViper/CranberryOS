#include "Print.h"

#include "MiniUart.h"

namespace
{
    /**
     * Output an integer to the specified output
     * 
     * @param aValue The value to output
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool OutputInteger(const uint64_t aValue, Print::Detail::OutputFunctorBase& arOutput)
    {
        auto success = true;
        if (aValue == 0)
        {
            success = arOutput.WriteChar('0');
        }
        else
        {
            // TODO
            // We'll need to make this more generic with various integer sizes, signed/unsigned, bases, etc

            // Stores each base-10 digit, from least to most significant
            static constexpr auto maxDigits = 32; // should be more than enough for 64 bit values
            uint8_t digits[maxDigits] = {0};
            auto numberOfDigits = 0u;

            auto remainingValue = aValue;
            while ((remainingValue > 0) && (numberOfDigits < maxDigits))
            {
                digits[numberOfDigits] = remainingValue % 10;
                remainingValue /= 10;
                ++numberOfDigits;
            }

            for (auto curDigit = 0u; (curDigit < numberOfDigits) && success; ++curDigit)
            {
                success = arOutput.WriteChar('0' + digits[numberOfDigits - curDigit - 1]);
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
         * @param aOutput The functor to use for outputting
         * 
         * @return True on success
         */
        bool DataWrapper<uint32_t>::OutputDataImpl(OutputFunctorBase& arOutput) const
        {
            return OutputInteger(WrappedData, arOutput);
        }

        /**
         * Output the data this wrapper holds to the given functor - overriden by implementation
         * 
         * @param aOutput The functor to use for outputting
         * 
         * @return True on success
         */
        bool DataWrapper<uint64_t>::OutputDataImpl(OutputFunctorBase& arOutput) const
        {
            return OutputInteger(WrappedData, arOutput);
        }

        /**
         * Output the data this wrapper holds to the given functor - overriden by implementation
         * 
         * @param aOutput The functor to use for outputting
         * 
         * @return True on success
         */
        bool DataWrapper<const char*>::OutputDataImpl(OutputFunctorBase& arOutput) const
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
            };

            if (apFormatString)
            {
                auto success = true;
                auto pcurChar = apFormatString;
                auto curState = ParseState::OutputCharacter;
                auto curDataElement = 0u;

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
                            curState = ParseState::CloseBrace;
                            writeChar = false;
                        }
                        break;

                    case ParseState::OpenBrace:
                        if (*pcurChar == '{')
                        {
                            // was an escaped open brace, so output the brace
                            curState = ParseState::OutputCharacter;
                        }
                        else if (*pcurChar == '}')
                        {
                            if (curDataElement < aDataCount)
                            {
                                // no need to null check the contents of apDataArray, since it's generated by functions
                                // that fill it with pointers to stack values
                                success = apDataArray[curDataElement]->OutputData(arOutput);
                            }
                            else
                            {
                                // TODO
                                // throw exception - invalid. For now, we output a placeholder
                                success = arOutput.WriteChar('{');
                                success = success && DataWrapper<decltype(curDataElement)>{curDataElement}.OutputData(arOutput);
                                success = success && arOutput.WriteChar('}');
                            }
                            
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