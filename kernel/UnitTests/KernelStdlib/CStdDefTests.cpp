#include <cstddef>
#include <cstdint>

// Using a lot of "magic numbers" in tests, so just silence the lint for the file
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

namespace UnitTests::KernelStdlib::CStdDef
{
    namespace
    {
        struct TestOffsetStruct
        {
            uint8_t ExpectedAt0;
            uint32_t ExpectedAt4;
            uint64_t ExpectedAt8;
        };

        static_assert(sizeof(std::size_t) == 8, "Unexpected size_t size");
        static_assert(sizeof(std::ptrdiff_t) == 8, "Unexpected ptrdiff_t size");
        static_assert(NULL == 0, "Unexpected NULL value");
        static_assert(alignof(std::max_align_t) == 16, "Unexpected max_align_t alignment");
        static_assert(offsetof(TestOffsetStruct, ExpectedAt0) == 0, "Unexpected offsetof first member");
        static_assert(offsetof(TestOffsetStruct, ExpectedAt4) == 4, "Unexpected offsetof second member");
        static_assert(offsetof(TestOffsetStruct, ExpectedAt8) == 8, "Unexpected offsetof third member");
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)