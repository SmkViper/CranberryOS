#include <cstdint>

namespace UnitTests::KernelStdlib::CStdInt
{
    namespace
    {
        static_assert(sizeof(std::int8_t) == 1, "Unexpected int8_t size");
        static_assert(sizeof(std::int16_t) == 2, "Unexpected int16_t size");
        static_assert(sizeof(std::int32_t) == 4, "Unexpected int32_t size");
        static_assert(sizeof(std::int64_t) == 8, "Unexpected int64_t size");

        static_assert(sizeof(std::intptr_t) == sizeof(void*), "Unexpected intptr_t size");

        static_assert(sizeof(std::uint8_t) == 1, "Unexpected uint8_t size");
        static_assert(sizeof(std::uint16_t) == 2, "Unexpected uint16_t size");
        static_assert(sizeof(std::uint32_t) == 4, "Unexpected uint32_t size");
        static_assert(sizeof(std::uint64_t) == 8, "Unexpected uint64_t size");

        static_assert(sizeof(std::uintptr_t) == sizeof(void*), "Unexpected uintptr_t size");

        static_assert(INT8_MIN == -128, "Unexpected int8 minimum");
        static_assert(INT16_MIN == -32768, "Unexpected int16 minimum");
        static_assert(INT32_MIN == -2147483648, "Unexpected int32 minimum");
        static_assert(INT64_MIN == -9223372036854775807LL - 1LL, "Unexpected int64 minimum"); // see note in stdint.h

        static_assert(INTPTR_MIN == -9223372036854775807LL - 1LL, "Unexpected intptr minimum"); // see note in stdint.h

        static_assert(INT8_MAX == 127, "Unexpected int8 maximum");
        static_assert(INT16_MAX == 32767, "Unexpected int16 maximum");
        static_assert(INT32_MAX == 2147483647, "Unexpected int32 maximum");
        static_assert(INT64_MAX == 9223372036854775807, "Unexpected int64 maximum");

        static_assert(INTPTR_MAX == 9223372036854775807, "Unexpected intptr maximum");

        static_assert(UINT8_MAX == 255, "Unexpected uint8 maximum");
        static_assert(UINT16_MAX == 65535, "Unexpected uint16 maximum");
        static_assert(UINT32_MAX == 4294967295, "Unexpected uint32 maximum");
        static_assert(UINT64_MAX == 18446744073709551615ULL, "Unexpected uint64 maximum");

        static_assert(UINTPTR_MAX == 18446744073709551615ULL, "Unexpected uintptr maximum");
    }
}