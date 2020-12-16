// TODO
// Figure out at some point how to rename this to just "exception" so it follows the C++ standard header naming convention

namespace std
{
    /**
     * Terminates execution
     */
    [[noreturn]] void terminate() noexcept;
}