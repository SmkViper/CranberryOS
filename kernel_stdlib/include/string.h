#ifndef __KERNEL_STDLIB_STRING_H__
#define __KERNEL_STDLIB_STRING_H__

#include <stddef.h>

extern "C"
{
    /**
     * Copies bytes from source to destination
     * 
     * @param apDest Pointer to the memory to copy to
     * @param apSource Pointer to memory to copy from
     * @param aCount Number of bytes to copy
     * 
     * @return The destination pointer
     */
    void* memcpy(void* apDest, const void* apSource, size_t aCount);

    /**
     * Fills the specified memory with the given value
     * 
     * @param apDest Pointer to the memory to fill
     * @param aChar The character to fill the memory with
     * @param aCount The number of bytes to fill
     * 
     * @return The given pointer
     */
    void* memset(void* apDest, int aChar, size_t aCount);

    /**
     * Compare two zero-terminated strings
     * 
     * @param apLHS Left-hand string to compare
     * @param apRHS Right-hand string to compare
     * 
     * @return 0 if they are equal, negative if LHS appears before RHS in lexicographical order, positive if LHS appears
     *         after RHS in lexicographical order
     */
    int strcmp(const char* apLHS, const char* apRHS);
}

#endif // __KERNEL_STDLIB_STRING_H__