#include "BitsetTests.h"

#include <bitset>
#include <cstdint>
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
        static_assert(~(std::bitset<64>{ 0b11 }[1]) == false, "Unexpected ~operator[] result");
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
        // #TODO: Find a good way to test swap on bool refs

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
        static_assert(~(std::bitset<32>{ 0b11 }[1]) == false, "Unexpected ~operator[] result");
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
        // #TODO: Find a good way to test swap on bool refs

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
        // #TODO: Find a good way to test swap on bool refs
    }

    void Run()
    {
        // #TODO: Runtime tests here
    }
}