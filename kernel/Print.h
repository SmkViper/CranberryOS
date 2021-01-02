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

        class MiniUARTOutputFunctor: public OutputFunctorBase
        {
        public:
            /**
             * Default constructor
             */
            MiniUARTOutputFunctor() = default;

        private:
            bool WriteCharImpl(char aChar) override;
        };

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
            DataWrapperBase(const DataWrapperBase& aOther) = delete;

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
            DataWrapperBase& operator=(const DataWrapperBase& aOther) = delete;

            /**
             * Output the data this wrapper holds to the given functor
             * 
             * @param aOutput The functor to use for outputting
             * 
             * @return True on success
             */
            bool OutputData(OutputFunctorBase& arOutput) const { return OutputDataImpl(arOutput); }

        private:
            /**
             * Output the data this wrapper holds to the given functor - overriden by implementation
             * 
             * @param aOutput The functor to use for outputting
             * 
             * @return True on success
             */
            virtual bool OutputDataImpl(OutputFunctorBase& arOutput) const = 0;
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
        class DataWrapper<uint32_t> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(const uint32_t aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(OutputFunctorBase& arOutput) const override;

            uint32_t WrappedData = 0u;
        };

        template<>
        class DataWrapper<const char*>: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(const char* const apData): pWrappedData{apData} {}

        private:
            bool OutputDataImpl(OutputFunctorBase& arOutput) const override;

            const char* pWrappedData = nullptr;
        };

        void FormatImpl(const char* apFormatString, OutputFunctorBase& arOutput, const DataWrapperBase** apDataArray, std::size_t aDataCount);
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
     */
    inline void FormatToMiniUART(const char* apFormatString)
    {
        Detail::MiniUARTOutputFunctor output;
        Detail::FormatImpl(apFormatString, output, nullptr, 0u);
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
    template<typename FirstArgType, typename... TailArgTypes>
    void FormatToMiniUART(const char* apFormatString, FirstArgType&& aFirstArg, TailArgTypes&&... aArgs)
    {
        // Lambda used so we can convert TailArgTypes to wrapped arguments, and then convert those to an array of
        // pointers to base. This basically lets us make sure the temporary wrappers last long enough for the call.
        auto outputArgs = [apFormatString] (const auto&... aWrappedArgs)
        {
            const Detail::DataWrapperBase* baseArgs[] = {&aWrappedArgs...};
            Detail::MiniUARTOutputFunctor output;
            Detail::FormatImpl(apFormatString, output, baseArgs, sizeof...(TailArgTypes) + 1);
        };

        outputArgs(Detail::DataWrapper<std::decay_t<FirstArgType>>{std::forward<FirstArgType>(aFirstArg)},
            Detail::DataWrapper<std::decay_t<TailArgTypes>>{std::forward<TailArgTypes>(aArgs)}...);
    }
}

#endif // KERNEL_PRINT_H