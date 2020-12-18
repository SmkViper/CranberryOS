#include <exception>

namespace std
{
    [[noreturn]] void terminate() noexcept
    {
        // This function is required for exception handling, so we implement something simple here to just freeze things up
        // TODO: Need something more visible for debugging reasons, most likely (i.e. something like a BSOD)
        while (true);
    }
}