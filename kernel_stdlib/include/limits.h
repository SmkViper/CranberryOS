// This is a "system" file, so we get to use reserved identifiers
// NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
// We are using defines because this is a C (not C++) header
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#ifndef __KERNEL_STDLIB_LIMITS_H__
#define __KERNEL_STDLIB_LIMITS_H__

// #TODO: This is only partial
// same as GCC/Clang's definition - relies on the compiler defines that have double underscores

#define CHAR_BIT    __CHAR_BIT__

#define SCHAR_MIN   (-__SCHAR_MAX__ - 1)
#define SCHAR_MAX   __SCHAR_MAX__
#define UCHAR_MAX   (__SCHAR_MAX__ * 2 + 1)

#ifdef __CHAR_UNSIGNED__
#define CHAR_MIN    0
#define CHAR_MAX    UCHAR_MAX
#else // __CHAR_UNSIGNED__
#define CHAR_MIN    SCHAR_MIN
#define CHAR_MAX    SCHAR_MAX
#endif // __CHAR_UNSIGNED__

#define SHRT_MIN    (-__SHRT_MAX__ - 1)
#define SHRT_MAX    __SHRT_MAX__
#define USHRT_MAX   (__SHRT_MAX__ * 2 + 1)

#define INT_MIN     (-__INT_MAX__ - 1)
#define INT_MAX     __INT_MAX__
#define UINT_MAX    (__INT_MAX__ * 2U + 1U)

#define LONG_MIN    (-__LONG_MAX__ - 1L)
#define LONG_MAX    __LONG_MAX__
#define ULONG_MAX   (__LONG_MAX__ * 2UL + 1UL)

#define LLONG_MIN   (-__LONG_LONG_MAX__ - 1LL)
#define LLONG_MAX   __LONG_LONG_MAX__
#define ULLONG_MAX  (__LONG_LONG_MAX__ * 2ULL + 1ULL)

#endif // __KERNEL_STDLIB_LIMITS_H__

// NOLINTEND(cppcoreguidelines-macro-usage)
// NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)