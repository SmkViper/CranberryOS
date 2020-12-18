#ifndef __KERNEL_STDLIB_STDINT_H__
#define __KERNEL_STDLIB_STDINT_H__

typedef signed char int8_t;
typedef short       int16_t;
typedef int         int32_t;
typedef long long   int64_t;

typedef long long   intptr_t;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef unsigned long long  uintptr_t;

// TODO: Fast/least typedefs
// TODO: intmax_t/uintmax_t typedefs

// Note that the literals for the smallest value are too large to fit in a type of said value, when interpreted as a
// positive value, since the compiler doesn't see "-x" as a unit, but "x" with negation. So instead, we use the largest
// signed value, negate it, and subtract 1 to get the smallest value

#define INT8_MIN (-0x7F - 1)
#define INT16_MIN (-0x7FFF - 1)
#define INT32_MIN (-0x7FFFFFFF - 1)
#define INT64_MIN (-0x7FFFFFFFFFFFFFFF - 1)

#define INTPTR_MIN (-0x7FFFFFFFFFFFFFFF - 1)

#define INT8_MAX 0x7F
#define INT16_MAX 0x7FFF
#define INT32_MAX 0x7FFFFFFF
#define INT64_MAX 0x7FFFFFFFFFFFFFFF

#define INTPTR_MAX 0x7FFFFFFFFFFFFFFF

#define UINT8_MAX 0xFF
#define UINT16_MAX 0xFFFF
#define UINT32_MAX 0xFFFFFFFF
#define UINT64_MAX 0xFFFFFFFFFFFFFFFF

#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFF

// TODO: Fast/least min/max defines
// TODO: intmax/uintmax min/max define

// TODO: minimum-width integer constants macros

#endif // __KERNEL_STDLIB_STDINT_H__