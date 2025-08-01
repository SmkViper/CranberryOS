#ifndef KERNEL_MEMORYMANAGEMENT_SIMPLEALLOCATOR_H
#define KERNEL_MEMORYMANAGEMENT_SIMPLEALLOCATOR_H

#include <cstddef>
#include <cstdint>

namespace MemoryManagement
{
    class SimpleAllocator
    {
    public:
        /**
         * Creates a simple allocator wrapping a single page of memory
         * 
         * @param apPage The page of memory to wrap, assumed to be page size
         */
        explicit SimpleAllocator(void* apPage);

        /**
         * Move constructor
         * 
         * @param amOther Other to move from
         */
        SimpleAllocator(SimpleAllocator&& amOther) noexcept;

        /**
         * Destructor - validates everything is freed and valid
         */
        ~SimpleAllocator();

        /**
         * Move assignment operator
         * 
         * @param amOther Other to move from
         * @return This allocator
         */
        SimpleAllocator& operator=(SimpleAllocator&& amOther) noexcept;

        // Disable copying
        SimpleAllocator(SimpleAllocator const&) = delete;
        SimpleAllocator& operator=(SimpleAllocator const&) = delete;

        /**
         * Attempts to allocate a chunk of the given size, with pointer alignment
         * 
         * @param aSize The amount of memory requested. If size is 0, null will be returned
         * @return The allocated memory, or null if allocation failed
         */
        [[nodiscard]] void* Malloc(std::size_t aSize);

        /**
         * Frees the memory pointed at by the given pointer, must be memory allocated by this allocator
         * 
         * @param apPtr The memory to free
         */
        void Free(void* apPtr);

    private:
        static constexpr std::uintptr_t FreeMagicCS = 0xFEFE'FEFE'FEFE'FEFEULL;
        static constexpr std::uintptr_t AllocatedMagicCS = 0xCDCD'CDCD'CDCD'CDCDULL;

        static bool ValidMagic(std::uintptr_t aPossibleMagic);

        struct BlockHeader
        {
            void SplitIfWorthIt(std::size_t aDesiredSize);
            [[nodiscard]] bool AttemptToMerge();
            [[nodiscard]] void* GetMemory();

            /**
             * Checks to see if this block is flagged as free
             * 
             * @return True if free
             */
            [[nodiscard]] bool IsFree() const { return Magic == FreeMagicCS; }

            // TODO: Might be worth ensuring that the memory of freed blocks is filled with a marker value to detect
            // memory corruption
            /**
             * Checks to see if this block is valid
             * 
             * @return True if valid
             */
            [[nodiscard]] bool IsValid() const { return ValidMagic(Magic); }

            std::size_t Size = 0;
            BlockHeader* pNextBlock = nullptr;
            std::uintptr_t Magic = FreeMagicCS; // used for both a free/allocated flag, and memory underflow detection
        };
        struct PageInfo
        {
            [[nodiscard]] bool ContainsPointer(void* apPtr) const;

            PageInfo* pNextPage = nullptr;
        };

        [[nodiscard]] static PageInfo* CreatePageInfo(void* apPage, PageInfo* apPrevPage);
        [[nodiscard]] static BlockHeader* CreateInitialFreeBlock(PageInfo* apEmptyPage);

        [[nodiscard]] bool PointerInAllocator(void* apPtr) const;
        [[nodiscard]] BlockHeader* FindFreeBlock(std::size_t aSize);
        void MergeFreeBlocks();

        PageInfo* pPageListHead = nullptr;
        BlockHeader* pBlockList = nullptr;
    };
} // MemoryManagement namespace

#endif // KERNEL_MEMORYMANAGEMENT_SIMPLEALLOCATOR_H