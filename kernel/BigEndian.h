#ifndef KERNEL_BIGENDIAN_H
#define KERNEL_BIGENDIAN_H

#include <cstdint>
#include <type_traits>
#include "Print.h"

template<typename IntegralT>
class BigEndian
{
public:
    // We want to be trivially_copyable because these are usually members of a struct we want to memcpy raw bytes into
    static_assert(std::is_trivially_copyable_v<IntegralT>, "Wrapped type is not trivially_copyable");

    /**
     * Default constructor - zero initializes
     */
    constexpr BigEndian() = default;

    /**
     * Conversion constructor from native value, intentionally implicit
     * 
     * @param aNativeValue The native value to convert
     */
    constexpr BigEndian(IntegralT const aNativeValue) // NOLINT(hicpp-explicit-conversions)
        : BEValue{ ToBigEndian(aNativeValue) }
    {}

    /**
     * Conversion operator to native value, intentionally implicit
     * 
     * @return The native value
     */
    constexpr operator IntegralT() const { return ToNative(BEValue); } // NOLINT(hicpp-explicit-conversions)

private:
    /**
     * Converts from native ordering to big endian
     * 
     * @param aNativeValue The native value to convert
     * @return The big endian value
     */
    static constexpr IntegralT ToBigEndian(IntegralT const aNativeValue)
    {
        // We assume native order is little endian
        return SwapByteOrder(aNativeValue);
    }

    /**
     * Converts from big endian ordering to native
     * 
     * @param aBEValue The big endian value to convert
     * @return The native value
     */
    static constexpr IntegralT ToNative(IntegralT const aBEValue)
    {
        // We assume native order is little endian
        return SwapByteOrder(aBEValue);
    }

    // Tiny helper to make sure the static assert doesn't evaluate until the template is instantiated.
    // #TODO: Probably can be moved somewhere else for common usage at some point
    template<typename T>
    static constexpr bool AlwaysFalse = false;

    /**
     * Swaps the byte order of the given value
     * 
     * @param aValue The value to swap the bytes of
     * @return The value with the bytes swapped
     */
    static constexpr IntegralT SwapByteOrder(IntegralT const aValue)
    {
        if constexpr (sizeof(IntegralT) == 1)
        {
            return aValue;
        }
        else if constexpr (sizeof(IntegralT) == sizeof(uint16_t))
        {
            return static_cast<IntegralT>(__builtin_bswap16(static_cast<uint16_t>(aValue)));
        }
        else if constexpr (sizeof(IntegralT) == sizeof(uint32_t))
        {
            return static_cast<IntegralT>(__builtin_bswap32(static_cast<uint32_t>(aValue)));
        }
        else if constexpr (sizeof(IntegralT) == sizeof(uint64_t))
        {
            return static_cast<IntegralT>(__builtin_bswap64(static_cast<uint64_t>(aValue)));
        }
        else
        {
            static_assert(AlwaysFalse<IntegralT>, "Unsupported byte swap size");
        }
    }

    IntegralT BEValue = 0;
};

namespace Print::Detail
{
    // #TODO: We're probably going to want to come up with a way to customize formatting output better, at least
    // without including the full Print header
    template<typename WrappedT>
    class DataWrapper<BigEndian<WrappedT>> final: public DataWrapperBase
    {
    public:
        /**
         * Wraps the specified data
         * 
         * @param aData Data to wrap
         */
        explicit DataWrapper(BigEndian<WrappedT> const aData): WrappedData{ aData } {}

    private:
        /**
         * Output the data this wrapper holds to the given functor - overriden by implementation
         * 
         * @param aFormat The format character to use for formatting
         * @param aOutput The functor to use for outputting
         * 
         * @return True on success
         */
        bool OutputDataImpl(char const /*aFormat*/, OutputFunctorBase& arOutput) const override
        {
            // #TODO: Going to want a better way to handle format so we can avoid formatting that doesn't make sense
            // #TODO: This is all very non-optimal, we'll want a better way to customize output for types

            // #TODO: Use std::array and numeric_limits::digits10 when we have that to resolve lint
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            char buffer[25] = {}; // should be enough for any 64 bit value (64*log(2) + zero terminator)

            Print::FormatToBuffer(buffer, "{}", static_cast<WrappedT>(WrappedData));
            Print::Detail::FormatVararg(static_cast<char const*>(buffer), arOutput);
            return true;
        }

        BigEndian<WrappedT> WrappedData;
    };
}

#endif // KERNEL_BIGENDIAN_H