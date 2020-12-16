#include "exception.h"

#include "../MiniUart.h"

namespace std
{
    [[noreturn]] void terminate() noexcept
    {
        // This function is required for exception handling, so we implement something simple here to just halt the core
        // and spit something out on MiniUART so we can see something went wrong.

        MiniUART::SendString("std::terminate() called\r\n");
        while (true); // just halt the core by looping forever
    }
}