#include <climits>
#include <cstdint>

namespace UnitTests::KernelStdlib::CLimits
{
    namespace
    {
        // These are obviously set up for a specific platform and will have to be updated if we ever port to one where
        // these are no longer true
        static_assert(CHAR_BIT == 8, "Unexpected CHAR_BIT"); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

        static_assert(SCHAR_MIN == INT8_MIN, "Unexpected SCHAR_MIN"); // NOLINT(misc-redundant-expression)
        static_assert(SCHAR_MAX == INT8_MAX, "Unexpected SCHAR_MAX");
        static_assert(UCHAR_MAX == UINT8_MAX, "Unexpected UCHAR_MAX");

        static_assert(CHAR_MIN == 0, "Unexpected CHAR_MIN");
        static_assert(CHAR_MAX == UCHAR_MAX, "Unexpected CHAR_MAX"); // NOLINT(misc-redundant-expression)

        static_assert(SHRT_MIN == INT16_MIN, "Unexpected SHRT_MIN"); // NOLINT(misc-redundant-expression)
        static_assert(SHRT_MAX == INT16_MAX, "Unexpected SHRT_MAX");
        static_assert(USHRT_MAX == UINT16_MAX, "Unexpected USHRT_MAX");

        static_assert(INT_MIN == INT32_MIN, "Unexpected INT_MIN"); // NOLINT(misc-redundant-expression)
        static_assert(INT_MAX == INT32_MAX, "Unexpected INT_MAX");
        static_assert(UINT_MAX == UINT32_MAX, "Unexpected UINT_MAX");

        static_assert(LONG_MIN == INT64_MIN, "Unexpected LONG_MIN");
        static_assert(LONG_MAX == INT64_MAX, "Unexpected LONG_MAX");
        static_assert(ULONG_MAX == UINT64_MAX, "Unexpected ULONG_MAX");

        static_assert(LLONG_MIN == INT64_MIN, "Unexpected LLONG_MIN");
        static_assert(LLONG_MAX == INT64_MAX, "Unexpected LLONG_MAX");
        static_assert(ULLONG_MAX == UINT64_MAX, "Unexpected ULLONG_MAX");
    }
}