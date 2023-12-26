#include "BitsetTests.h"

#include <bitset>
#include <cstdint>
#include <utility>
#include "../Framework.h"

namespace UnitTests::KernelStdlib::Bitset
{
    namespace
    {
        /**
         * Helper for testing operator &= for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aOtherValue The other value to &= with it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetAndAssign(unsigned long const aInitialValue, unsigned long const aOtherValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            std::bitset<BitsetSize> other{ aOtherValue };
            return result &= other;
        }

        /**
         * Helper for testing operator & for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aOtherValue The other value to & with it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetAnd(unsigned long const aInitialValue, unsigned long const aOtherValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            std::bitset<BitsetSize> other{ aOtherValue };
            return result & other;
        }

        /**
         * Helper for testing operator |= for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aOtherValue The other value to |= with it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetOrAssign(unsigned long const aInitialValue, unsigned long const aOtherValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            std::bitset<BitsetSize> other{ aOtherValue };
            return result |= other;
        }

        /**
         * Helper for testing operator | for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aOtherValue The other value to | with it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetOr(unsigned long const aInitialValue, unsigned long const aOtherValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            std::bitset<BitsetSize> other{ aOtherValue };
            return result | other;
        }

        /**
         * Helper for testing operator ^= for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aOtherValue The other value to ^= with it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetXorAssign(unsigned long const aInitialValue, unsigned long const aOtherValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            std::bitset<BitsetSize> other{ aOtherValue };
            return result ^= other;
        }

        /**
         * Helper for testing operator ^ for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aOtherValue The other value to ^ with it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetXor(unsigned long const aInitialValue, unsigned long const aOtherValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            std::bitset<BitsetSize> other{ aOtherValue };
            return result ^ other;
        }

        /**
         * Helper for testing operator [] (set) for std::bitset
         * 
         * @param aInitialValue The initial value of the bitset
         * @param aPos The bit to adjust
         * @param aValue The value to give it
         * @return The resulting bitset
         */
        template<std::size_t BitsetSize>
        constexpr auto BitsetIndexSet(unsigned long const aInitialValue, size_t const aPos, bool const aValue)
        {
            std::bitset<BitsetSize> result{ aInitialValue };
            result[aPos] = aValue;
            return result;
        }

        /**
         * Helper for testing bit reference assignment
         * 
         * @param aDestPos The destination bit
         * @param aSourcePos The source bit
         * @return The value of the destination bit, which should be true
         */
        template<std::size_t BitsetSize>
        constexpr bool BitReferenceAssign(size_t const aDestPos, size_t const aSourcePos)
        {
            std::bitset<BitsetSize> value;
            value[aSourcePos] = true;
            value[aDestPos] = value[aSourcePos];
            return value[aDestPos];
        }

        /**
         * Helper for testing bit reference flip()
         * 
         * @param aPos Bit to flip
         * @return Whether the bit flipped to true and false successfully
         */
        template<std::size_t BitsetSize>
        constexpr bool BitReferenceFlip(size_t const aPos)
        {
            std::bitset<BitsetSize> value;
            value[aPos].flip();
            auto const expectedTrue = static_cast<bool>(value[aPos]);
            value[aPos].flip();
            auto const expectedFalse = static_cast<bool>(value[aPos]);

            return expectedTrue && !expectedFalse;
        }

        /**
         * Helper for testing swap on bit references to the same container type
         * 
         * @param aPos Bit to swap
         * @return Whether the swap happened successfully
         */
        template<std::size_t BitsetSize>
        constexpr bool BitReferenceSameSizeSwap(size_t const aPos)
        {
            using std::swap;

            std::bitset<BitsetSize> value1, value2;
            value1[aPos] = true;

            swap(value1[aPos], value2[aPos]);

            return !value1[aPos] && value2[aPos];
        }

        /**
         * Helper for testing swap on bit references to different container type
         * 
         * @param aPos Bit to swap
         * @return Whether the swap happened successfully
         */
        template<std::size_t BitsetSize>
        constexpr bool BitReferenceDifferentSizeSwap(size_t const aPos)
        {
            using std::swap;

            std::bitset<BitsetSize> value1;
            value1[aPos] = true;

            std::bitset<16> value2;
            constexpr unsigned value2Bit = 5;
            swap(value1[aPos], value2[value2Bit]);

            return !value1[aPos] && value2[value2Bit];
        }

        /**
         * Helper for testing swap on bit references with a bool
         * 
         * @param aPos Bit to swap
         * @return Whether the swap happened successfully
         */
        template<std::size_t BitsetSize>
        constexpr bool BitReferenceBoolSwap(size_t const aPos)
        {
            using std::swap;

            std::bitset<BitsetSize> value1;
            value1[aPos] = true;

            bool otherBit = false;
            swap(otherBit, value1[aPos]);
            auto const firstPassed = otherBit && !value1[aPos];

            swap(value1[aPos], otherBit);
            auto const secondPassed = !otherBit && value1[aPos];

            return firstPassed && secondPassed;
        }

        /**
         * Helper for testing const bit reference conversion from non-const
         * 
         * @param aPos The position of the bit to set
         * @return The value of the supposedly set bit
         */
        template<std::size_t BitsetSize>
        constexpr bool ConstBitReferenceConstruct(size_t const aPos)
        {
            std::bitset<BitsetSize> value;
            value[aPos] = true;

            typename decltype(value)::const_reference constBitRef = value[aPos];
            return constBitRef;
        }

        /**
         * Helper for testing const bit reference conversion to bool
         * 
         * @param aPos The position of the bit to set
         * @return The value of the supposedly set bit
         */
        template<std::size_t BitsetSize>
        constexpr bool ConstBitReferenceBool(size_t const aPos)
        {
            std::bitset<BitsetSize> value;
            value[aPos] = true;

            std::bitset<BitsetSize> const& constValue = value;
            return constValue[aPos];
        }

        /**
         * Helper for testing const bit reference operator~
         * 
         * @param aPos The position of the bit to set
         * @return The NOTed-value of the supposedly set bit
         */
        template<std::size_t BitsetSize>
        constexpr bool ConstBitReferenceNot(size_t const aPos)
        {
            std::bitset<BitsetSize> value;
            value[aPos] = true;

            std::bitset<BitsetSize> const& constValue = value;
            return ~constValue[aPos];
        }

        // An empty bitset
        static_assert(sizeof(std::bitset<0>) == 1, "Unexpected size of empty bitset");
        static_assert(std::bitset<0>{}.to_ulong() == 0UL, "Expected constructor for bitset to zero");
        static_assert(std::bitset<0>{ 10 }.to_ulong() == 0UL, "Expected constructor for bitset to truncate");
        static_assert(BitsetAndAssign<0>(0b11, 0b101).to_ulong() == 0, "Unexpected &= result");
        static_assert(BitsetOrAssign<0>(0b11, 0b101).to_ulong() == 0, "Unexpected |= result");
        static_assert(BitsetXorAssign<0>(0b11, 0b101).to_ulong() == 0, "Unexpected ^= result");
        // #TODO: Can't test set on bitset<0> because any bit is out of range
        static_assert((~std::bitset<0>{ 0b11 }).to_ulong() == 0, "Unexpected ~ result");
        static_assert(std::bitset<0>{ 0b11 }.flip().to_ulong() == 0, "Unexpected flip() result");
        // #TODO: Can't test flip(pos) on bitset<0> because any bit is out of range
        // We don't test operator[] on bitset<0> because it's UB when out of range (doesn't throw)
        static_assert(std::bitset<0>{ 10 }.to_ullong() == 0ULL, "Unexpected to_ullong() result");
        static_assert(std::bitset<0>{}.size() == 0, "Unexpected size() result");
        // #TODO: Can't test test() on bitset<0> because any bit is out of range
        static_assert(std::bitset<0>{ 10 }.all() == true, "Unexpected all() result");
        static_assert(std::bitset<0>{ 10 }.any() == false, "Unexpected any() result");
        static_assert(std::bitset<0>{ 10 }.none() == true, "Unexpected none() result");
        static_assert(BitsetAnd<0>(0b11, 0b101).to_ulong() == 0, "Unexpected & result");
        static_assert(BitsetOr<0>(0b11, 0b101).to_ulong() == 0, "Unexpected | result");
        static_assert(BitsetXor<0>(0b11, 0b101).to_ulong() == 0, "Unexpected ^ result");
        
        // We don't test bit references for bitset<0> because it has no bits

        // A bitset that is exactly the right size for its internal type
        static_assert(sizeof(std::bitset<64>) == 8, "Unexpected size of full bitset");
        static_assert(std::bitset<64>{}.to_ulong() == 0UL, "Expected constructor for bitset to zero");
        static_assert(std::bitset<64>{ 10 }.to_ulong() == 10UL, "Expected constructor for bitset to set low bits");
        static_assert(BitsetAndAssign<64>(0b11, 0b101).to_ulong() == 0b001, "Unexpected &= result");
        static_assert(BitsetOrAssign<64>(0b11, 0b101).to_ulong() == 0b111, "Unexpected |= result");
        static_assert(BitsetXorAssign<64>(0b11, 0b101).to_ulong() == 0b110, "Unexpected ^= result");
        static_assert(std::bitset<64>{ 0b11 }.set(2, true).to_ulong() == 0b111, "Unexpected set() result");
        static_assert(std::bitset<64>{ 0b11 }.set(1, false).to_ulong() == 0b001, "Unexpected set() result");
        static_assert(std::bitset<64>{ 0b11 }.reset(1).to_ulong() == 0b001, "Unexpected reset() result");
        static_assert((~std::bitset<64>{ 0b11 }).to_ulong() == ~0b11UL, "Unexpected ~ result");
        static_assert(std::bitset<64>{ 0b11 }.flip().to_ulong() == ~0b11UL, "Unexpected flip() result");
        static_assert(std::bitset<64>{ 0b11 }.flip(1).to_ulong() == 0b01, "Unexpected flip(pos) result");
        static_assert(std::bitset<64>{ 0b11 }.flip(2).to_ulong() == 0b111, "Unexpected flip(pos) result");
        static_assert(std::bitset<64>{ 0b11 }[1] == true, "Unexpected operator[] result");
        static_assert(BitsetIndexSet<64>(0b11, 2, true).to_ulong() == 0b111, "Unexpected operator[] (set) result");
        static_assert(BitsetIndexSet<64>(0b11, 1, false).to_ulong() == 0b001, "Unexpected operator[] (set) result");
        static_assert(std::bitset<64>{ 10 }.to_ullong() == 10ULL, "Unexpected to_ullong() result");
        static_assert(std::bitset<64>{}.size() == 64, "Unexpected size() result");
        static_assert(std::bitset<64>{ 0b11 }.test(1) == true, "Unexpected test() result");
        static_assert(std::bitset<64>{ 0b11 }.test(2) == false, "Unexpected test() result");
        static_assert(std::bitset<64>{ 10 }.all() == false, "Unexpected all() result");
        static_assert(std::bitset<64>{ UINT64_MAX }.all() == true, "Unexpected all() result");
        static_assert(std::bitset<64>{ 0 }.any() == false, "Unexpected any() result");
        static_assert(std::bitset<64>{ 10 }.any() == true, "Unexpected any() result");
        static_assert(std::bitset<64>{ 0 }.none() == true, "Unexpected none() result");
        static_assert(std::bitset<64>{ 10 }.none() == false, "Unexpected none() result");
        static_assert(BitsetAnd<64>(0b11, 0b101).to_ulong() == 0b001, "Unexpected & result");
        static_assert(BitsetOr<64>(0b11, 0b101).to_ulong() == 0b111, "Unexpected | result");
        static_assert(BitsetXor<64>(0b11, 0b101).to_ulong() == 0b110, "Unexpected ^ result");

        // Reference tests for bitset<64>
        // operator bool tested by index tests above
        static_assert(~(std::bitset<64>{ 0b11 }[1]) == false, "Unexpected bit reference ~ result");
        // operator =(bool) tested by index tests above
        static_assert(BitReferenceAssign<64>(10, 1), "Unexpected bit reference operator=(reference) result");
        static_assert(BitReferenceFlip<64>(10), "Unexpected bit reference flip() result");
        static_assert(BitReferenceSameSizeSwap<64>(10), "Unexpected bit reference swap same size result");
        static_assert(BitReferenceDifferentSizeSwap<64>(10), "Unexpected bit reference swap different size result");
        static_assert(BitReferenceBoolSwap<64>(10), "Unexpected bit reference swap bool& result");
        static_assert(ConstBitReferenceConstruct<64>(10), "Unexpected const bit reference conversion result");
        static_assert(ConstBitReferenceBool<64>(10), "Unexpected const bit reference bool() result");
        static_assert(ConstBitReferenceNot<64>(10) == false, "Unexpected const bit reference operator~ result");

        // A bitset that is smaller than its internal type
        static_assert(sizeof(std::bitset<32>) == 8, "Unexpected size of partial bitset");
        static_assert(std::bitset<32>{}.to_ulong() == 0UL, "Expected constructor for bitset smaller than word size to zero");
        static_assert(std::bitset<32>{ 10 }.to_ulong() == 10UL, "Expected constructor for bitset smaller than word size to set low bits");
        static_assert(std::bitset<32>{ UINT64_MAX }.to_ulong() == UINT32_MAX, "Expected constructor for bitset smaller than word size to truncate large input");
        static_assert(BitsetAndAssign<32>(0b11, 0b101).to_ulong() == 0b001, "Unexpected &= result");
        static_assert(BitsetOrAssign<32>(0b11, 0b101).to_ulong() == 0b111, "Unexpected |= result");
        static_assert(BitsetXorAssign<32>(0b11, 0b101).to_ulong() == 0b110, "Unexpected ^= result");
        static_assert(std::bitset<32>{ 0b11 }.set(2, true).to_ulong() == 0b111, "Unexpected set() result");
        static_assert(std::bitset<32>{ 0b11 }.set(1, false).to_ulong() == 0b001, "Unexpected set() result");
        static_assert(std::bitset<32>{ 0b11 }.reset(1).to_ulong() == 0b001, "Unexpected reset() result");
        static_assert((~std::bitset<32>{ 0b11 }).to_ulong() == ~0b11U, "Unexpected ~ result");
        static_assert(std::bitset<32>{ 0b11 }.flip().to_ulong() == ~0b11U, "Unexpected flip() result");
        static_assert(std::bitset<32>{ 0b11 }.flip(1).to_ulong() == 0b01, "Unexpected flip(pos) result");
        static_assert(std::bitset<32>{ 0b11 }.flip(2).to_ulong() == 0b111, "Unexpected flip(pos) result");
        static_assert(std::bitset<32>{ 0b11 }[1] == true, "Unexpected operator[] result");
        static_assert(BitsetIndexSet<32>(0b11, 2, true).to_ulong() == 0b111, "Unexpected operator[] (set) result");
        static_assert(BitsetIndexSet<32>(0b11, 1, false).to_ulong() == 0b001, "Unexpected operator[] (set) result");
        static_assert(std::bitset<32>{ 10 }.to_ullong() == 10ULL, "Unexpected to_ullong() result");
        static_assert(std::bitset<32>{}.size() == 32, "Unexpected size() result");
        static_assert(std::bitset<32>{ 0b11 }.test(1) == true, "Unexpected test() result");
        static_assert(std::bitset<32>{ 0b11 }.test(2) == false, "Unexpected test() result");
        static_assert(std::bitset<32>{ 10 }.all() == false, "Unexpected all() result");
        static_assert(std::bitset<32>{ UINT32_MAX }.all() == true, "Unexpected all() result"); // makes sure that the top 32 bits don't count
        static_assert(std::bitset<32>{ 0 }.any() == false, "Unexpected any() result");
        static_assert(std::bitset<32>{ 10 }.any() == true, "Unexpected any() result");
        static_assert(std::bitset<32>{ 0 }.none() == true, "Unexpected none() result");
        static_assert(std::bitset<32>{ 10 }.none() == false, "Unexpected none() result");
        static_assert(BitsetAnd<32>(0b11, 0b101).to_ulong() == 0b001, "Unexpected & result");
        static_assert(BitsetOr<32>(0b11, 0b101).to_ulong() == 0b111, "Unexpected | result");
        static_assert(BitsetXor<32>(0b11, 0b101).to_ulong() == 0b110, "Unexpected ^ result");

        // Reference tests for bitset<32>
        // operator bool tested by index tests above
        static_assert(~(std::bitset<32>{ 0b11 }[1]) == false, "Unexpected bit reference ~ result");
        // operator =(bool) tested by index tests above
        static_assert(BitReferenceAssign<32>(10, 1), "Unexpected bit reference operator=(reference) result");
        static_assert(BitReferenceFlip<32>(10), "Unexpected bit reference flip() result");
        static_assert(BitReferenceSameSizeSwap<32>(10), "Unexpected bit reference swap same size result");
        static_assert(BitReferenceDifferentSizeSwap<32>(10), "Unexpected bit reference swap different size result");
        static_assert(BitReferenceBoolSwap<32>(10), "Unexpected bit reference swap bool& result");
        static_assert(ConstBitReferenceConstruct<32>(10), "Unexpected const bit reference conversion result");
        static_assert(ConstBitReferenceBool<32>(10), "Unexpected const bit reference bool() result");
        static_assert(ConstBitReferenceNot<32>(10) == false, "Unexpected const bit reference operator~ result");

        // A bitset that is larger than its internal type, partially filled
        static_assert(sizeof(std::bitset<96>) == 16, "Unexpected size of large bitset");
        static_assert(std::bitset<96>{}.to_ulong() == 0UL, "Expected constructor for bitset larger than word size to zero");
        static_assert(std::bitset<96>{ 10 }.to_ulong() == 10UL, "Expected constructor for bitset larger than word size to set low bits");
        // #TODO: Need a good way to test high numbers with large bitsets, since to_ulong won't return the right value for
        // any bits over 64 (cause it's returning an unsigned long). And when we put in the range checking it'll throw
        // anyway
        static_assert(std::bitset<96>{ 0 }.set(65, true)[65] == true, "Unexpected set() result");
        static_assert(std::bitset<96>{ 0 }.flip(65)[65] == true, "Unexpected flip(pos) result");
        // operator[] (get) being tested above
        static_assert(BitsetIndexSet<96>(0, 65, true)[65] == true, "Unexpected operator[] (set) result");
        // to_ulong tested above
        static_assert(std::bitset<96>{ 10 }.to_ullong() == 10ULL, "Unexpected to_ullong result");
        static_assert(std::bitset<96>{}.size() == 96, "Unexpected size() result");
        static_assert(std::bitset<96>{ 0 }.set(65).test(65) == true, "Unexpected test() result");
        static_assert(std::bitset<96>{ 0 }.set(65).test(66) == false, "Unexpected test() result");
        static_assert(std::bitset<96>{ 10 }.all() == false, "Unexpected all() result");
        static_assert(std::bitset<96>{ UINT64_MAX }.all() == false, "Unexpected all() result");
        static_assert(std::bitset<96>{ 0 }.flip().all() == true, "Unexpected all() result");
        static_assert(std::bitset<96>{ 0 }.any() == false, "Unexpected any() result");
        static_assert(std::bitset<96>{ 0 }.set(65, true).any() == true, "Unexpected any() result");
        static_assert(std::bitset<96>{ 0 }.none() == true, "Unexpected none() result");
        static_assert(std::bitset<96>{ 10 }.none() == false, "Unexpected none() result");

        // Reference tests for bitset<96>
        // operator bool tested by index tests above
        static_assert(~(std::bitset<96>{ 0 }.set(65, true)[65]) == false, "Unexpected bit reference ~ result");
        // operator =(bool) tested by index tests above
        static_assert(BitReferenceAssign<96>(85, 90), "Unexpected bit reference operator=(reference) result");
        static_assert(BitReferenceFlip<96>(85), "Unexpected bit reference flip() result");
        static_assert(BitReferenceSameSizeSwap<96>(85), "Unexpected bit reference swap same size result");
        static_assert(BitReferenceDifferentSizeSwap<96>(85), "Unexpected bit reference swap different size result");
        static_assert(BitReferenceBoolSwap<96>(85), "Unexpected bit reference swap bool& result");
        static_assert(ConstBitReferenceConstruct<96>(85), "Unexpected const bit reference conversion result");
        static_assert(ConstBitReferenceBool<96>(85), "Unexpected const bit reference bool() result");
        static_assert(ConstBitReferenceNot<96>(85) == false, "Unexpected const bit reference operator~ result");
    }

    void Run()
    {
        // #TODO: No tests yet, probably will want some when we have exceptions implemented for the things that throw
    }
}