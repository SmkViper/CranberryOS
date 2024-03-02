#ifndef KERNEL_UNITTESTS_FRAMEWORK_H
#define KERNEL_UNITTESTS_FRAMEWORK_H

#include "../Print.h"

namespace UnitTests
{
    namespace Details
    {
        void EmitTestResultImpl(bool aResult, char const* apMessage);
        void EmitTestSkipResultImpl(char const* apMessage);
    }

    /**
     * Emits pass or failure based on a result bool
     * 
     * @param aResult The result of the test
     * @param apMessage The message to emit (may be a format string)
     * @param aArgs Arguments to substitute into the format string
     */
    template<typename... ArgT>
    void EmitTestResult(bool const aResult, char const* const apMessage, ArgT&&... aArgs)
    {
        if constexpr (sizeof...(ArgT) == 0)
        {
            // Don't bother formatting if we have nothing to format
            Details::EmitTestResultImpl(aResult, apMessage);
        }
        else
        {
            static constexpr size_t bufferSize = 256;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            char buffer[bufferSize];
            ::Print::FormatToBuffer(buffer, apMessage, std::forward<ArgT>(aArgs)...);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            Details::EmitTestResultImpl(aResult, buffer);
        }
    }

    /**
     * Emits a skip message
     * 
     * @param apMessage The message to emit (may be a format string)
     * @param aArgs Arguments to substitute into the format string
     */
    template<typename... ArgT>
    void EmitTestSkipResult(char const* const apMessage, ArgT&&... aArgs)
    {
        if constexpr (sizeof...(ArgT) == 0)
        {
            // Don't bother formatting if we have nothing to format
            Details::EmitTestSkipResultImpl(apMessage);
        }
        else
        {
            static constexpr size_t bufferSize = 256;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            char buffer[bufferSize];
            ::Print::FormatToBuffer(buffer, apMessage, std::forward<ArgT>(aArgs)...);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            Details::EmitTestSkipResultImpl(buffer);
        }
    }

    /**
     * Runs kernel unit tests. Assumes MiniUART is set up for output and static
     * constructors have been run
     */
    void Run();

    /**
     * Runs kernel unit tests to be run after static destructors are run
     */
    void RunPostStaticDestructors();
}

#endif // KERNEL_UNITTESTS_FRAMEWORK_H