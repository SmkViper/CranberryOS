#include "SimpleAllocator.h"

#include <bit>
#include <cstddef>
#include <cstdint>
// Technically needed for placement new, but for some reason clang-tidy doesn't pick up on that
#include <new> // NOLINT(misc-include-cleaner)
#include "../Debug.h"
#include "../MemoryManager.h"

namespace MemoryManagement
{
    namespace
    {
        /**
         * Calculates the size of a free block that covers a whole page
         * 
         * @return The size of the free block
         */
        template<typename TPageHeader, typename TBlockHeader>
        constexpr std::size_t FullPageBlockSize()
        {
            return MemoryManager::PageSize - sizeof(TPageHeader) - sizeof(TBlockHeader);
        }
    } // anonymous namespace

    SimpleAllocator::SimpleAllocator(void* const apPage)
        : pPageListHead{ CreatePageInfo(apPage, nullptr /*no previous page*/) }
        , pBlockList{ CreateInitialFreeBlock(pPageListHead) }
    {
        // #TODO: Should we own the page and be responsible for freeing it? Probably. In which case update comments
        // around other NOLINT in this file. Alternatively, maybe a wrapping class that owns the page could be written
        // so we have options for owning and non-owning allocators.
    }

    SimpleAllocator::SimpleAllocator(SimpleAllocator&& amOther) noexcept
        : pPageListHead{ amOther.pPageListHead }
        , pBlockList{ amOther.pBlockList }
    {
        amOther.pPageListHead = nullptr;
        amOther.pBlockList = nullptr;
    }

    SimpleAllocator::~SimpleAllocator()
    {
        // handle null page list head in case we're moved-from
        if (pPageListHead != nullptr)
        {
            // #TODO: Logic here will need to change when we support multiple pages
            if (pPageListHead->pNextPage != nullptr)
            {
                Debug::Panic("Not expecting to support multiple pages yet!");
            }
            // because of how we're set up, if everything is freed then we'll just have a block list consisting of page-
            // sized free blocks
            auto const* pcurBlock = pBlockList;
            while (pcurBlock != nullptr)
            {
                if (!pcurBlock->IsValid())
                {
                    Debug::Panic("Corrupted block in list!");
                }
                if (!pcurBlock->IsFree())
                {
                    Debug::Panic("Unfreed memory in list!");
                }
                if (pcurBlock->Size != FullPageBlockSize<PageInfo, BlockHeader>())
                {
                    Debug::Panic("Expected to only have page-sized free blocks!");
                }
                pcurBlock = pcurBlock->pNextBlock;
            }
        }
    }

    SimpleAllocator& SimpleAllocator::operator=(SimpleAllocator&& amOther) noexcept
    {
        using std::swap;
        swap(pPageListHead, amOther.pPageListHead);
        swap(pBlockList, amOther.pBlockList);
        return *this;
    }

    void* SimpleAllocator::Malloc(size_t const aSize)
    {
        // #TODO: Likely going to want to eventually support custom alignment
        constexpr auto defaultAlignmentC = alignof(void*);
        static_assert((sizeof(BlockHeader) % defaultAlignmentC) == 0,
            "Block header needs to be a multiple of alignment for malloc to guarantee alignment"
        );

        void* pretVal = nullptr;
        if (aSize > 0)
        {
            if (pPageListHead == nullptr)
            {
                Debug::Panic("Cannot allocate from an allocator with no pages!");
            }

            // we need to adjust the size so that if we're splitting the block the next block ends up on the right
            // alignment
            auto const sizePlusAlign = aSize + (defaultAlignmentC -(aSize % defaultAlignmentC));
            
            if (auto* const pexistingFreeBlock = FindFreeBlock(sizePlusAlign); pexistingFreeBlock != nullptr)
            {
                pexistingFreeBlock->SplitIfWorthIt(sizePlusAlign);
                pexistingFreeBlock->Magic = AllocatedMagicCS;
                // #TODO: Might be worth validating the memory here against a known magic to detect corruption
                pretVal = pexistingFreeBlock->GetMemory();
            }
            // #TODO: Eventually support requesting another memory page
        }
        return pretVal;
    }

    void SimpleAllocator::Free(void* const apPtr)
    {
        // allow a free of null to succeed immediately
        if (apPtr == nullptr)
        {
            return;
        }

        if (pPageListHead == nullptr)
        {
            Debug::Panic("Allocator with no pages cannot free!");
        }

        if (!PointerInAllocator(apPtr))
        {
            Debug::Panic("Tried to free pointer with wrong allocator!");
        }
        // block header is right before the pointer itself
        auto* const pheader = std::bit_cast<BlockHeader*>(apPtr) - 1;
        if (!pheader->IsValid())
        {
            Debug::Panic("Memory corruption in block, OR pointer is not at start of block!");
        }
        if (pheader->IsFree())
        {
            Debug::Panic("Attempted to double-free memory!");
        }

        pheader->Magic = FreeMagicCS;
        MergeFreeBlocks();
    }

    /**
     * Checks to see if the given magic value is a valid one
     * 
     * @param aPossibleMagic Magic to check
     * @return True if it is one of the valid values
     */
    bool SimpleAllocator::ValidMagic(std::uintptr_t const aPossibleMagic)
    {
        return (aPossibleMagic == FreeMagicCS) || (aPossibleMagic == AllocatedMagicCS);
    }

    /**
     * Splits this block into two, ensuring that the block has the given size, and the new block has whatever is
     * remaining
     * 
     * @param aDesiredSize The amount of memory this block should now use (new block gets whatever is left)
     */
    void SimpleAllocator::BlockHeader::SplitIfWorthIt(std::size_t const aDesiredSize)
    {
        if (Size < aDesiredSize)
        {
            Debug::Panic("Block size not large enough for desired size!");
        }
        if (!IsFree())
        {
            Debug::Panic("Attempted to split a non-free block!");
        }

        // we'll consider a block worth splitting if it results in enough space for another block header and a pointer
        if (Size >= (aDesiredSize + sizeof(BlockHeader) + sizeof(void*)))
        {
            auto* const pnewBlockMemory = std::bit_cast<uint8_t*>(GetMemory()) + aDesiredSize;
            // caller is responsible for freeing, not us, and they're getting a raw pointer, not our block
            auto* const pnewBlock = new (pnewBlockMemory) BlockHeader{ // NOLINT(cppcoreguidelines-owning-memory)
                .Size = Size - aDesiredSize - sizeof(BlockHeader),
                .pNextBlock = pNextBlock,
                .Magic = FreeMagicCS
            };

            pNextBlock = pnewBlock;
            Size = aDesiredSize;
        }
    }

    /**
     * Attempt to merge this block with its next block, if it is free
     * 
     * @return True if a block was merged
     */
    bool SimpleAllocator::BlockHeader::AttemptToMerge()
    {
        auto blockMerged = false;
        if (pNextBlock == nullptr)
        {
            return blockMerged;
        }
        if (!IsFree())
        {
            Debug::Panic("Attempted to merge a non-free block!");
        }

        // #TODO: Logic needs to be updated when we support more than one page (since we don't want to necessarily
        // merge blocks across pages - unless the pages are adjacent and we can "merge" the pages in some way)
        auto const expectedNextBlockPtr = std::bit_cast<uintptr_t>(GetMemory()) + Size;
        if (expectedNextBlockPtr != std::bit_cast<uintptr_t>(pNextBlock))
        {
            Debug::Panic("Next block isn't at expected position!");
        }
        if (pNextBlock->IsFree())
        {
            auto* const poriginalNextBlock = pNextBlock;
            pNextBlock = pNextBlock->pNextBlock;

            Size += sizeof(BlockHeader) + poriginalNextBlock->Size;
            // #TODO: std::destroy when we have it
            poriginalNextBlock->~BlockHeader();

            blockMerged = true;
        }
        return blockMerged;
    }

    /**
     * Get the memory the block is pointing at
     * 
     * @return The memory the block points at
     */
    void* SimpleAllocator::BlockHeader::GetMemory()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return std::bit_cast<void*>(this + 1);
    }

    /**
     * Check to see if the pointer is inside this page
     * 
     * @param apPtr Pointer to check
     * @return True if in this page somewhere
     */
    bool SimpleAllocator::PageInfo::ContainsPointer(void* const apPtr) const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto const pageStart = std::bit_cast<uintptr_t>(this);
        auto const pageEnd = pageStart + MemoryManager::PageSize;
        auto const ptrInteger = std::bit_cast<uintptr_t>(apPtr);
        return (ptrInteger >= pageStart) && (ptrInteger < pageEnd);
    }

    /**
     * Creates a page info for the given page
     * 
     * @param apPage Memory page to make the info for
     * @param apPrevPage The previous page to this new one
     * @return The page info for the empty page
     */
    auto SimpleAllocator::CreatePageInfo(void* const apPage, PageInfo* const apPrevPage) -> PageInfo*
    {
        // We're testing here instead of requiring a reference since we're called from the constructor which takes a
        // page pointer
        // #TODO: Maybe pass page by reference to constructor?
        if (apPage == nullptr)
        {
            Debug::Panic("Cannot create a page info for a null page!");
        }

        auto* const pnextPage = (apPrevPage != nullptr) ? apPrevPage->pNextPage : nullptr;
        // we're placement newing into a page that is owned by our caller, so we don't need to do any explicit
        // ownership here
        auto* const pnewPage = new (apPage) PageInfo{ // NOLINT(cppcoreguidelines-owning-memory)
            .pNextPage = pnextPage
        };
        if (apPrevPage != nullptr)
        {
            apPrevPage->pNextPage = pnewPage;
        }
        return pnewPage;
    }

    /**
     * Creates the initial free block representing the whole page
     * 
     * @param apEmptyPage The page that is free
     * @return The free block for the page
     */
    auto SimpleAllocator::CreateInitialFreeBlock(PageInfo* const apEmptyPage) -> BlockHeader*
    {
        // The empty page we're given is assumed to be page sized, have nothing in it, and its memory to be stored
        // the page info struct itself
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto* const ppageMemoryStart = std::bit_cast<void*>(apEmptyPage + 1);
        // we're placement newing into a page that is owned by our caller, so we don't need to do any explicit
        // ownership here
        return new (ppageMemoryStart) BlockHeader{ // NOLINT(cppcoreguidelines-owning-memory)
            .Size = FullPageBlockSize<PageInfo, BlockHeader>(), 
            .pNextBlock = nullptr,
            .Magic = FreeMagicCS
        };
    }

    /**
     * Checks to see if this pointer could possibly have come from this allocator
     * 
     * @param apPtr Pointer to check
     * @return True if it might have come from this allocator, false if it definitely did not
     */
    bool SimpleAllocator::PointerInAllocator(void* apPtr) const
    {
        // Simple sanity check to see if the pointer could theoretically have come from this allocator
        auto const* pcurPage = pPageListHead;
        auto containsPointer = false;
        while (!containsPointer && (pcurPage != nullptr))
        {
            containsPointer = pcurPage->ContainsPointer(apPtr);
            pcurPage = pcurPage->pNextPage;
        }
        return containsPointer;
    }

    /**
     * Finds a free block of the given size or larger
     * 
     * @param aSize Size of block needed
     * @return A block of the given size or larger, or null of none found
     */
    auto SimpleAllocator::FindFreeBlock(std::size_t const aSize) -> BlockHeader*
    {
        auto* pcurBlock = pBlockList;
        auto* pretVal = static_cast<BlockHeader*>(nullptr);
        while ((pcurBlock != nullptr) && (pretVal == nullptr))
        {
            if (!ValidMagic(pcurBlock->Magic))
            {
                Debug::Panic("Invalid magic on memory block - probable memory corruption!");
            }
            if (pcurBlock->IsFree() && (pcurBlock->Size >= aSize))
            {
                pretVal = pcurBlock;
            }
            pcurBlock = pcurBlock->pNextBlock;
        }
        return pretVal;
    }

    /**
     * Goes through the list of blocks and merges any adjacent free ones
     */
    void SimpleAllocator::MergeFreeBlocks()
    {
        auto* pcurBlock = pBlockList;
        while (pcurBlock != nullptr)
        {
            if (pcurBlock->IsFree() && pcurBlock->AttemptToMerge())
            {
                // merged a free block - don't advance pcurBlock since it might be able to be merged again
            }
            else
            {
                // block wasn't free, or didn't merge, so go to next one
                pcurBlock = pcurBlock->pNextBlock;
            }
        }
    }
} // MemoryManagement namespace