#ifndef KERNEL_MEMORY_MANAGER_H
#define KERNEL_MEMORY_MANAGER_H

namespace MemoryManager
{
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