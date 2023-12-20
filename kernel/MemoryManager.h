#ifndef KERNEL_MEMORY_MANAGER_H
#define KERNEL_MEMORY_MANAGER_H

#include <cstdint>
#include "AArch64/MemoryDescriptor.h"

namespace Scheduler
{
    struct TaskStruct;
}

namespace MemoryManager
{
    // #TODO These should be special types indicating the type of pointer
    constexpr uintptr_t KernalVirtualAddressStart = 0xFFFF'0000'0000'0000;
    constexpr uintptr_t DeviceBaseAddress = 0x3F00'0000;

    // sizes depend on how many bits the descriptor uses to index into pages or tables
    constexpr size_t PageSize = 1ULL << AArch64::Descriptor::PageOffsetBits;
    constexpr size_t L2BlockSize = 1ULL << (AArch64::Descriptor::PageOffsetBits + AArch64::Descriptor::TableIndexBits);

    // the number of pointers in a single table is based on how many bits we have to index the table
    constexpr size_t PointersPerTable = 1ULL << AArch64::Descriptor::TableIndexBits;
    static_assert(PointersPerTable * sizeof(AArch64::Descriptor::Fault) == PageSize, "Expected to be able to fit a table into a page");
    
    /**
     * Allocates a page of memory in the kernel virtual address space
     * 
     * @return The address of the page in kernel VA space
     */
    void* AllocateKernelPage();

    /**
     * Allocates a page of memory in the task's virtual address space that contains the specified address
     * 
     * @param arTask The task that will hold the page
     * @param aVirtualAddress The address that the page should start on
     * @return The newly allocated page in kernal virtual address space
     */
    void* AllocateUserPage(Scheduler::TaskStruct& arTask, uintptr_t aVirtualAddress);

    /**
     * Copies the virtual memory from the source task into the destination task
     * 
     * @param arDestinationTask The task to copy the memory into
     * @param aCurrentTask The task to copy the memory from (assumed to be the current task)
     */
    bool CopyVirtualMemory(Scheduler::TaskStruct& arDestinationTask, const Scheduler::TaskStruct& aCurrentTask);

    /**
     * Set the current page global directory
     * 
     * @param apNewPGD Pointer to the new page global directory
     */
    void SetPageGlobalDirectory(const void* apNewPGD);

    /**
     * Calculate the start of the block of the given size containing the given pointer
     * 
     * @param aPtr The pointer inside the block
     * @param aBlockSize The size of the block (must be power of 2)
     * 
     * @return The address of the start of the block containing the pointer
    */
    constexpr uintptr_t CalculateBlockStart(uintptr_t const aPtr, size_t const aBlockSize)
    {
        // #TODO Confirm block size is a power of 2
        return aPtr & (~(aBlockSize - 1));
    }

    /**
     * Calculate the end of the block of the given size containing the given pointer
     * 
     * @param aPtr The pointer inside the block
     * @param aBlockSize The size of the block (must be power of 2)
     * 
     * @return The last address in the block containing the pointer (adding 1 would be the byte starting the next
     * block)
    */
    constexpr uintptr_t CalculateBlockEnd(uintptr_t const aPtr, size_t const aBlockSize)
    {
        // #TODO Confirm block size is a power of 2
        return CalculateBlockStart(aPtr, aBlockSize) + aBlockSize - 1;
    }
}

#endif // KERNEL_MEMORY_MANAGER_H