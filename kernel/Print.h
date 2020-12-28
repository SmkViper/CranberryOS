#ifndef KERNEL_PRINT_H
#define KERNEL_PRINT_H

#include <cstdint>

namespace Print
{
    // implementation details, do not use directly
    namespace Detail
    {
        class OutputFunctorBase;

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
            explicit DataWrapper(uint32_t aData): WrappedData{aData} {}

        private:
            bool OutputDataImpl(OutputFunctorBase& arOutput) const override;

            uint32_t WrappedData;
        };

        void FormatToMiniUARTImpl(const char* apFormatString, const DataWrapperBase* apDataArray, uint32_t aDataCount);
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
        Detail::FormatToMiniUARTImpl(apFormatString, nullptr, 0u);
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
    template<typename FirstArgType, typename... ArgTypes>
    void FormatToMiniUART(const char* apFormatString, FirstArgType&& aFirstArg, ArgTypes&&... aArgs)
    {
        // Lambda used so we can convert ArgTypes to wrapped arguments, and then convert those to an array of pointers
        // to base. This basically lets us make sure the temporary wrappers last long enough for the call.
        auto outputArgs = [apFormatString] (const Detail::DataWrapper<ArgTypes>&... aWrappedArgs)
        {
            const Detail::DataWrapperBase* baseArgs[] = {aWrappedArgs...};
            Detail::FormatToMiniUARTImpl(apFormatString, baseArgs, sizeof...(ArgTypes));
        };

        outputArgs(Detail::DataWrapper<FirstArgType>(aFirstArg), Detail::DataWrapper<ArgTypes>(aArgs)...);
    }
}

#endif // KERNEL_PRINT_H