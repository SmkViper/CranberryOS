#ifndef __KERNEL_STDLIB_STRING_H__
#define __KERNEL_STDLIB_STRING_H__

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

#endif // __KERNEL_STDLIB_STRING_H__