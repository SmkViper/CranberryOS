#include <new>

// #TODO: Eventually need a real implementation. For now, we just need something for the linker to use

void operator delete([[maybe_unused]] void* __apBlock) noexcept
{
    // #TODO: Implement
}

void operator delete[]([[maybe_unused]] void* __apBlock) noexcept
{
    // #TODO: Implement
}