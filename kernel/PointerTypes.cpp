#include "PointerTypes.h"

#include "Print.h"

namespace Print::Detail
{
    /**
     * Output the data this wrapper holds to the given functor - overriden by implementation
     * 
     * @param aFormat The format character to use for formatting
     * @param aOutput The functor to use for outputting
     * 
     * @return True on success
     */
    bool DataWrapper<PhysicalPtr>::OutputDataImpl(char const /*aFormat*/, OutputFunctorBase& arOutput) const
    {
        // #TODO: Going to want a better way to handle format so we can avoid formatting that doesn't make sense
        // #TODO: This is all very non-optimal, we'll want a better way to customize output for types

        // #TODO: Use std::array and numeric_limits::digits10 when we have that to resolve lint
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        char buffer[20] = {}; // should be enough for any 64 bit value + prefix (16 digits + 0x prefix + P prefix)

        // #TODO: We'll want to have the output padded with zeroes when we add support for that
        Print::FormatToBuffer(buffer, "P{:x}", WrappedData.GetAddress());

        // #TODO: std::array can handle this lint when we have it
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        Print::Detail::FormatVararg(buffer, arOutput);
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
    bool DataWrapper<VirtualPtr>::OutputDataImpl(char const /*aFormat*/, OutputFunctorBase& arOutput) const
    {
        // #TODO: Going to want a better way to handle format so we can avoid formatting that doesn't make sense
        // #TODO: This is all very non-optimal, we'll want a better way to customize output for types

        // #TODO: Use std::array and numeric_limits::digits10 when we have that to resolve lint
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        char buffer[20] = {}; // should be enough for any 64 bit value + prefix (16 digits + 0x prefix + P prefix)

        // #TODO: We'll want to have the output padded with zeroes when we add support for that
        Print::FormatToBuffer(buffer, "V{:x}", WrappedData.GetAddress());

        // #TODO: std::array can handle this lint when we have it
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        Print::Detail::FormatVararg(buffer, arOutput);
        return true;
    }
}