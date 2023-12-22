#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace MemoryMappedIO
{
    /**
     * Put a 32 bit data value into the given address
     * 
     * @param aAddress Address to store data into
     * @param aData Data to store
     */
    void Put32(uintptr_t aAddress, uint32_t aData);

    /**
     * Obtain a 32 bit data value from the given address
     * 
     * @param aAddress Address to read data from
     * @return The value store there
     */
    uint32_t Get32(uintptr_t aAddress);
}

namespace Timing
{
    /**
     * Delay by busy-looping until the counter expires
     * 
     * @param aCount Value to count down to 0 (cycle count)
     */
    void Delay(uint64_t aCount);

    /**
     * Obtain the current clock frequency in hertz
     * 
     * @return The current clock frequency in Hz
     */
    uint32_t GetSystemCounterClockFrequencyHz();
}

/**
 * Helper class so lambdas can be used with visit-like interfaces
 */
template<class... Ts> struct Overloaded : Ts...
{ using Ts::operator()... ; };

template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

/**
 * Writes a multi-bit value to a bitset
 * 
 * @param arBitset The bitset to modify
 * @param aValue The value to write
 * @param aMask An un-shifted mask for the bits to write
 * @param aShit How much to shift the value before writing
 */
template<typename SourceType, size_t BitsetSize>
void WriteMultiBitValue(std::bitset<BitsetSize>& arBitset, SourceType const aValue, uint64_t const aMask, uint64_t const aShift)
{
    static_assert(sizeof(SourceType) <= sizeof(uint64_t), "SourceType too large");
    arBitset &= std::bitset<64>{ ~(aMask << aShift) };
    auto const maskedValue = (static_cast<uint64_t>(aValue) & aMask) << aShift;
    arBitset |= std::bitset<64>{ maskedValue };
}

/**
 * Reads a multi-bit value from a bitset
 * 
 * @param aBitset The bitset to read
 * @param aMask An un-shifted mask fro the bits to read
 * @param aShift How much to shift the value after reading
 * @return The read bits, casted to SourceType
 */
template<typename SourceType, size_t BitsetSize>
SourceType ReadMultiBitValue(std::bitset<BitsetSize> const& aBitset, uint64_t const aMask, uint64_t const aShift)
{
    static_assert(sizeof(SourceType) <= sizeof(uint64_t), "SourceType too large");
    auto const shiftedMask = aMask << aShift;
    return static_cast<SourceType>((aBitset & std::bitset<64>{ shiftedMask }).to_ulong() >> aShift);
}

#endif // KERNEL_UTILS_H