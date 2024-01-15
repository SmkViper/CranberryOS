#ifndef KERNEL_PRINT_H
#define KERNEL_PRINT_H

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace Print
{
    // implementation details, do not use directly
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
            OutputFunctorBase(OutputFunctorBase const& aOther) = delete;

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
            OutputFunctorBase& operator=(OutputFunctorBase const& aOther) = delete;

            /**
             * Write out a single character to the output
             * 
             * @param aChar The character to write
             * 
             * @return True on success
             */
            bool WriteChar(char const aChar) { return WriteCharImpl(aChar); }

        protected:
            /**
             * Default constructor
             */
            OutputFunctorBase() = default;

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

        class MiniUARTOutputFunctor final: public OutputFunctorBase
        {
        public:
            /**
             * Default constructor
             */
            MiniUARTOutputFunctor() = default;

        private:
            bool WriteCharImpl(char aChar) override;
        };

        template<std::size_t BufferSize>
        class StaticBufferOutputFunctor final: public OutputFunctorBase
        {
        public:
            /**
             * Constructor with a buffer
             * 
             * @param apBuffer Buffer to write to - assumed to be BufferSize in size and non-null
             */
            explicit StaticBufferOutputFunctor(char (&apBuffer)[BufferSize]): pBuffer{apBuffer} {}

            /**
             * Obtains the number of characters written to the buffer
             * 
             * @return Number of characters written
             */
            std::size_t GetCharsWritten() const {return CurWritePos;}

        private:
            /**
             * Write out a single character to the output - overridden by implementation
             * 
             * @param aChar The character to write
             * 
             * @return True on success
             */
            bool WriteCharImpl(char const aChar) override
            {
                auto success = (CurWritePos < BufferSize);
                if (success)
                {
                    pBuffer[CurWritePos] = aChar;
                    ++CurWritePos;
                }
                return success;
            }

            char* pBuffer = nullptr;
            std::size_t CurWritePos = 0u;
        };
        template<std::size_t BufferSize>
        StaticBufferOutputFunctor(char (&arBuffer)[BufferSize]) -> StaticBufferOutputFunctor<BufferSize>;

        /**
         * Base we can pass to our output function that will output the data it holds to a given functor
         */
        class DataWrapperBase
        {
        public:
            /**
             * Default constructor
             */
            DataWrapperBase() = default;

            /**
             * Copy constructor
             * 
             * @param aOther Wrapper to copy
             */
            DataWrapperBase(DataWrapperBase const& aOther) = delete;

            /**
             * Destructor
             */
            virtual ~DataWrapperBase() = default;

            /**
             * Assignment operator
             * 
             * @param aOther Wrapper to copy
             * 
             * @return This data wrapper
             */
            DataWrapperBase& operator=(DataWrapperBase const& aOther) = delete;

            /**
             * Output the data this wrapper holds to the given functor
             * 
             * @param aFormat The format character to use for formatting
             * @param aOutput The functor to use for outputting
             * 
             * @return True on success
             */
            bool OutputData(char const aFormat, OutputFunctorBase& arOutput) const { return OutputDataImpl(aFormat, arOutput); }

        private:
            /**
             * Output the data this wrapper holds to the given functor - overriden by implementation
             * 
             * @param aFormat The format character to use for formatting
             * @param aOutput The functor to use for outputting
             * 
             * @return True on success
             */
            virtual bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const = 0;
        };

        template<typename Type>
        class DataWrapper: public DataWrapperBase
        {
            // Tiny helper to make sure the static assert doesn't evaluate until the template is instantiated.
            // TODO
            // Probably can be moved somewhere else for common usage at some point
            template<typename T>
            static constexpr bool AlwaysFalse = false;

            static_assert(AlwaysFalse<Type>, "Unsupported type for Format");
        };

        template<>
        class DataWrapper<uint8_t> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(uint8_t const aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            uint8_t WrappedData = 0u;
        };

        template<>
        class DataWrapper<uint16_t> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(uint16_t const aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            uint16_t WrappedData = 0u;
        };

        template<>
        class DataWrapper<uint32_t> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(uint32_t const aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            uint32_t WrappedData = 0u;
        };

        template<>
        class DataWrapper<uint64_t> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(uint64_t const aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            uint64_t WrappedData = 0u;
        };

        template<>
        class DataWrapper<size_t> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(size_t const aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            size_t WrappedData = 0u;
        };

        template<>
        class DataWrapper<char const*>: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(char const* const apData): pWrappedData{apData} {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            char const* pWrappedData = nullptr;
        };

        template<>
        class DataWrapper<char*>: public DataWrapper<char const*>
        {
        public:
            using DataWrapper<char const*>::DataWrapper;
        };

        void FormatImpl(char const* apFormatString, OutputFunctorBase& arOutput, DataWrapperBase const* const* apDataArray, std::size_t aDataCount);

        /**
         * Helper to output format string using the given output functor
         * 
         * @param apFormatString String to output
         * @param arOutput Functor to send the string to
         */
        inline void FormatVararg(char const* apFormatString, OutputFunctorBase& arOutput)
        {
            FormatImpl(apFormatString, arOutput, nullptr, 0u);
        }

        /**
         * Helper to output format string using the given output functor
         * 
         * @param apFormatString String to output
         * @param arOutput Functor to send the string to
         * @param aFirstArg The first argument to substitute
         * @param aArgs The remaining arguments to substitute
         */
        template<typename FirstArgType, typename... TailArgTypes>
        void FormatVararg(char const* apFormatString, OutputFunctorBase& arOutput, FirstArgType&& aFirstArg, TailArgTypes&&... aArgs)
        {
            // Lambda used so we can convert FirstArgType and TailArgTypes to wrapped arguments, and then convert those
            // to an array of pointers to base. This basically lets us make sure the temporary wrappers last long
            // enough for the call.
            auto outputArgs = [apFormatString, &arOutput] (auto const&... aWrappedArgs)
            {
                Detail::DataWrapperBase const* baseArgs[] = {&aWrappedArgs...};
                FormatImpl(apFormatString, arOutput, baseArgs, sizeof...(TailArgTypes) + 1 /* +1 for FirstArgType*/);
            };

            outputArgs(Detail::DataWrapper<std::decay_t<FirstArgType>>{std::forward<FirstArgType>(aFirstArg)},
                Detail::DataWrapper<std::decay_t<TailArgTypes>>{std::forward<TailArgTypes>(aArgs)}...);
        }
    }

    /**
     * Format the given string and data, passing it to MiniUART. The format string follows a subset of the
     * std::format specification.
     * 
     * - Regular characters (except '{' and '}') are output as-is
     * - "{{" and "}}" are escape sequences for '{' and '}' respectively
     * - Data elements are output in-order, whenever a "{}" pair is found
     * 
     * Indexing and format specifications are not currently supported
     * 
     * @param apFormatString The format string, following the std::format syntax
     * @param aArgs The arguments to substitute into the string
     */
    template<typename... ArgTypes>
    void FormatToMiniUART(char const* apFormatString, ArgTypes&&... aArgs)
    {
        Detail::MiniUARTOutputFunctor output;
        Detail::FormatVararg(apFormatString, output, std::forward<ArgTypes>(aArgs)...);
    }

    /**
     * Format the given string and data, writing to a buffer. The format string follows a subset of the std::format
     * specification.
     * 
     * - Regular characters (except '{' and '}') are output as-is
     * - "{{" and "}}" are escape sequences for '{' and '}' respectively
     * - Data elements are output in-order, whenever a "{}" pair is found
     * 
     * Indexing and format specifications are not currently supported
     * 
     * @param arBuffer Buffer to write to
     * @param apFormatString The format string, following the std::format syntax
     * @param aArgs The arguments to substitute into the string
     */
    template<std::size_t BufferSize, typename... ArgTypes>
    void FormatToBuffer(char (&arBuffer)[BufferSize], char const* apFormatString, ArgTypes&&... aArgs)
    {
        Detail::StaticBufferOutputFunctor output{arBuffer};
        Detail::FormatVararg(apFormatString, output, std::forward<ArgTypes>(aArgs)...);

        auto const charsWritten = output.GetCharsWritten();
        const auto zeroPos = (charsWritten < BufferSize) ? charsWritten : (BufferSize - 1);
        arBuffer[zeroPos] = '\0';
    }
}

#endif // KERNEL_PRINT_H