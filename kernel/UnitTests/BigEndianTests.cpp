#include "../BigEndian.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "../Print.h"
#include "Framework.h"

namespace UnitTests::BigEndian
{
    namespace
    {
        // Disable some lints for this file since they don't make sense here
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

        static_assert(::BigEndian<uint8_t>{} == 0, "Constructor didn't make zeroed value (uint8_t)");
        static_assert(std::is_trivially_copyable_v<::BigEndian<uint8_t>>, "BigEndian<uint8_t> is not trivially copyable");
        static_assert(::BigEndian<uint8_t>{ 0x05 } == 0x05, "Failed to store and retrive value (uint8_t)");
        static_assert(std::bit_cast<uint8_t>(::BigEndian<uint8_t>{ 0x05 }) == 0x05, "Failed to store value as big endian (uint8_t)");

        static_assert(::BigEndian<uint16_t>{} == 0, "Constructor didn't make zeroed value (uint16_t)");
        static_assert(std::is_trivially_copyable_v<::BigEndian<uint16_t>>, "BigEndian<uint16_t> is not trivially copyable");
        static_assert(::BigEndian<uint16_t>{ 0xABCD } == 0xABCD, "Failed to store and retrive value (uint16_t)");
        static_assert(std::bit_cast<uint16_t>(::BigEndian<uint16_t>{ 0xABCD }) == 0xCDAB, "Failed to store value as big endian (uint16_t)");

        static_assert(::BigEndian<uint32_t>{} == 0, "Constructor didn't make zeroed value (uint32_t)");
        static_assert(std::is_trivially_copyable_v<::BigEndian<uint32_t>>, "BigEndian<uint32_t> is not trivially copyable");
        static_assert(::BigEndian<uint32_t>{ 0xABCD'1234 } == 0xABCD'1234, "Failed to store and retrive value (uint32_t)");
        static_assert(std::bit_cast<uint32_t>(::BigEndian<uint32_t>{ 0xABCD'1234 }) == 0x3412'CDAB, "Failed to store value as big endian (uint32_t)");

        static_assert(::BigEndian<uint64_t>{} == 0, "Constructor didn't make zeroed value (uint64_t)");
        static_assert(std::is_trivially_copyable_v<::BigEndian<uint64_t>>, "BigEndian<uint64_t> is not trivially copyable");
        static_assert(::BigEndian<uint64_t>{ 0xABCD'EF00'1234'5678 } == 0xABCD'EF00'1234'5678, "Failed to store and retrive value (uint64_t)");
        static_assert(std::bit_cast<uint64_t>(::BigEndian<uint64_t>{ 0xABCD'EF00'1234'5678 }) == 0x7856'3412'00EF'CDAB, "Failed to store value as big endian (uint64_t)");

        // #TODO: Figure out a way to test the static assert for types BigEndian doesn't support

        /**
         * Testing BigEndian<uint8_t> formatting
         */
        void BigEndianUInt8PrintTest()
        {
            char buffer[256]; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            Print::FormatToBuffer(buffer, "{}", ::BigEndian<uint8_t>{ 0x05 });
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            EmitTestResult(strcmp(buffer, "5") == 0, "BigEndian<uint8_t> print format");
        }

        /**
         * Testing BigEndian<uint16_t> formatting
         */
        void BigEndianUInt16PrintTest()
        {
            char buffer[256]; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            Print::FormatToBuffer(buffer, "{}", ::BigEndian<uint16_t>{ 0x1234 });
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            EmitTestResult(strcmp(buffer, "4660") == 0, "BigEndian<uint16_t> print format");
        }

        /**
         * Testing BigEndian<uint32_t> formatting
         */
        void BigEndianUInt32PrintTest()
        {
            char buffer[256]; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            Print::FormatToBuffer(buffer, "{}", ::BigEndian<uint32_t>{ 0xABCD'1234 });
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            EmitTestResult(strcmp(buffer, "2882343476") == 0, "BigEndian<uint32_t> print format");
        }

        /**
         * Testing BigEndian<uint64_t> formatting
         */
        void BigEndianUInt64PrintTest()
        {
            char buffer[256]; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            Print::FormatToBuffer(buffer, "{}", ::BigEndian<uint64_t>{ 0xABCD'EF00'1234'5678 });
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            EmitTestResult(strcmp(buffer, "12379813734295819896") == 0, "BigEndian<uint64_t> print format");
        }

        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    }

    // #TODO: Figure out if there is a better way to fix this
    // Unclear why clang-tidy thinks internal linkage works here, as it's called from Framework.cpp
    // NOLINTNEXTLINE(misc-use-internal-linkage)
    void Run()
    {
        BigEndianUInt8PrintTest();
        BigEndianUInt16PrintTest();
        BigEndianUInt32PrintTest();
        BigEndianUInt64PrintTest();
    }
}