#include <array>

// Using a lot of "magic numbers" in tests, so just silence the lint for the file
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

namespace UnitTests::KernelStdlib::Array
{
    namespace
    {
        // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
        constexpr int testArray[] = {1, 2, 3};
        constexpr char testString[] = "Hello";
        // NOLINTEND(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

        static_assert(std::size(testArray) == 3, "Unexpected size result for int array");
        static_assert(std::size(testString) == 6, "Unexpected size result for string");
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)