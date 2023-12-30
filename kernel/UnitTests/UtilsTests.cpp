#include "UtilsTests.h"

#include "Framework.h"
#include "../Utils.h"

namespace UnitTests::Utils
{
    namespace
    {
        // #TODO: No obvious way to test MemoryMappedIO stuff yet
        // #TODO: No obvious way to test Timing stuff yet

        /**
         * Test the Overloaded struct
         */
        void OverloadedTest()
        {
            bool intCalled = false;
            bool floatCalled = false;

            auto const testObj = Overloaded{
                [&intCalled] (int) { intCalled = true; },
                [&floatCalled] (float) { floatCalled = true; }
            };

            testObj(10);
            testObj(1.5f);

            EmitTestResult(intCalled && floatCalled, "Overloaded::operator()");
        }

        /**
         * Ensure WriteMultiBitValue and ReadMultiBitValue set and read the expected bits
         */
        void ReadWriteMultiBitValueTest()
        {
            static constexpr uint64_t inputValue = 0xABCD; // larger than the mask to ensure we're masking
            static constexpr uint64_t mask = 0xFF;
            static constexpr uint64_t shift = 3;

            std::bitset<64> bitset;
            WriteMultiBitValue(bitset, inputValue, mask, shift);
            EmitTestResult(bitset.to_ullong() == 0x0000'0000'0000'0668, "Write multi bit value mask/shift");
            EmitTestResult(ReadMultiBitValue<uint64_t>(bitset, mask, shift) == 0xCD, "Read/write multi bit value round-trip");
        }

        /**
         * Ensure WriteMultiBitValue and ReadMultiBitValue work with enum values
         */
        void ReadWriteMultiBitEnumTest()
        {
            enum class TestEnum: uint8_t
            {
                Value = 0b101
            };
            static constexpr uint64_t mask = 0b111;
            static constexpr uint64_t shift = 3;

            std::bitset<64> bitset;
            WriteMultiBitValue(bitset, TestEnum::Value, mask, shift);
            EmitTestResult(bitset.to_ullong() == 0x0000'0000'0000'0028, "Write multi bit enum mask/shift");
            EmitTestResult(ReadMultiBitValue<TestEnum>(bitset, mask, shift) == TestEnum::Value, "Read/write multi bit enum round-trip");
        }
    }

    void Run()
    {
        OverloadedTest();

        ReadWriteMultiBitValueTest();
        ReadWriteMultiBitEnumTest();
    }
}