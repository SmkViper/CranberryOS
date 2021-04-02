#ifndef KERNEL_MEMORY_MANAGER_H
#define KERNEL_MEMORY_MANAGER_H

#include <cstdint>
#include "ARM/MMUDefines.h"

namespace MemoryManager
{
    constexpr uintptr_t KernalVirtualAddressStart = VA_START;
    /**
     * Allocate a page of memory
     * 
     * @return Pointer to the allocated page of memory
     */
    void* GetFreePage();

    /**
     * Free a page of memory
     * 
     * @param apPage The page to free
     */
    void FreePage(void* apPage);
}

#endif // KERNEL_MEMORY_MANAGER_H