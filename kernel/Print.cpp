#include "Print.h"

#include "MiniUart.h"

namespace Print
{
    namespace Detail
    {
        /**
         * A functor base class for outputting a character somewhere, since we don't have an allocator for a
         * std::function implementation
         */
        class OutputFunctorBase
        {
        public:
            /**
             * Copy constructor
             * 
             * @param aOther Functor to copy
             */
            OutputFunctorBase(const OutputFunctorBase& aOther) = delete;

            /**
             * Destructor
             */
            virtual ~OutputFunctorBase() = default;

            /**
             * Assignment operator
             * 
             * @param aOther Functor to copy
             * 
             * @return This functor
             */
            OutputFunctorBase& operator=(const OutputFunctorBase& aOther) = delete;

            /**
             * Write out a single character to the output
             * 
             * @param aChar The character to write
             * 
             * @return True on success
             */
            bool WriteChar(const char aChar) { return WriteCharImpl(aChar); }

        private:
            /**
             * Write out a single character to the output - overridden by implementation
             * 
             * @param aChar The character to write
             * 
             * @return True on success
             */
            virtual bool WriteCharImpl(char aChar) = 0;
        };

        /**
         * Output the data this wrapper holds to the given functor - overriden by implementation
         * 
         * @param aOutput The functor to use for outputting
         * 
         * @return True on success
         */
        bool DataWrapper<uint32_t>::OutputDataImpl(OutputFunctorBase& arOutput) const
        {
            // TODO
            return arOutput.WriteChar('?');
        }

        /**
         * Formats the string and sends it to MiniUART
         * 
         * @param apFormatString The format string, following the std::format syntax
         * @param apDataArray First element in an array of elements to substitute into the format string
         * @param aDataCount Number of items in the data array
         */
        void FormatToMiniUARTImpl(const char* const apFormatString, const DataWrapperBase* const /*apDataArray*/, const uint32_t /*aDataCount*/)
        {
            // TODO
            MiniUART::SendString(apFormatString);
        }
    }
}