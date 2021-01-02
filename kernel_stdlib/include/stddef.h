#ifndef __KERNEL_STDLIB_STDDEF_H__
#define __KERNEL_STDLIB_STDDEF_H__

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#define NULL ((void*)0)

// same as GCC/Clang's definition
typedef struct
{
    long long __max_align_part1 __attribute__((__aligned__(__alignof__(long long))));
    long double __max_align_part2 __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif // __KERNEL_STDLIB_STDDEF_H__