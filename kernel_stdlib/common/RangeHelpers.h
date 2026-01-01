// This is a "system" file, so we get to use reserved identifiers
// NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
// And we can modify the std namespace, because we're defining it
// NOLINTBEGIN(cert-dcl58-cpp)

#ifndef __KERNEL_STDLIB_COMMON_RANGEHELPERS_H__
#define __KERNEL_STDLIB_COMMON_RANGEHELPERS_H__

#include <cstddef>

namespace std
{
    template<typename _ElementT, size_t _ElementCount>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    [[nodiscard]] constexpr size_t size(_ElementT const (&/*arArray*/)[_ElementCount]) noexcept
    {
        return _ElementCount;
    }
}

#endif // __KERNEL_STDLIB_COMMON_RANGEHELPERS_H__

// NOLINTEND(cert-dcl58-cpp)
// NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)