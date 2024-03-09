#include "user_Program.h"

#include <cstdint>
#include "user_SystemCall.h"

namespace
{
    // We need to force all our strings into the .rodata.user segment so the kernel code that sets up the user process
    // successfully grabs them. Otherwise we'll trigger a memory exception when trying to access data in kernel code
    // #TODO: Not sure why this is needed for when LTO is on, but not when it is off. Maybe it's consolidating strings
    // that match across all compilation units, and therefore losing the section that these files have?
    // #TODO: Can't switch to pointers because that seems to lose the section definitions for the data itself
    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    __attribute__((section(".rodata.user")))
    const char UserProcessStr[] = "User process\r\n";
    __attribute__((section(".rodata.user")))
    const char ForkErrStr[] = "Error during fork\r\n";
    __attribute__((section(".rodata.user")))
    const char LoopParentStr[] = "abcde";
    __attribute__((section(".rodata.user")))
    const char LoopChildStr[] = "12345";
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

    /**
     * Delay for a certain number of cycles
     * 
     * @param aCount Number of cycles
     */
    __attribute__((section(".text.user")))
    void Delay(uint64_t const aCount)
    {
        for (auto i = 0U; i < aCount; ++i)
        {
            // make sure the compiler doesn't optimize out the decrementing so that this is
            // an actual cycle count delay
            // NOLINTNEXTLINE(hicpp-no-assembler)
            asm volatile("nop");
        }
    }

    /**
     * Our main program loop, just outputs a string to Write one character at a time with a delay
     * 
     * @param apStr The string to output
     */
    __attribute__((section(".text.user")))
    void Loop(const char* const apStr)
    {
        // #TODO: We don't have std::array yet, so suppress lint warning
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
        char buffer[] = {'\0', '\0'};
        while (true)
        {
            // #TODO: Could be made safer with something like string_view perhaps when we have that
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            for (auto curIndex = 0U; apStr[curIndex] != '\0'; ++curIndex)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                buffer[0] = apStr[curIndex];
                SystemCall::Write(static_cast<char const*>(buffer));
                constexpr auto delayDuration = 100'000U;
                Delay(delayDuration);
            }
        }
    }
} // anonymous namespace

namespace User
{
    __attribute__((section(".text.user")))
    void Process()
    {
        SystemCall::Write(static_cast<char const*>(UserProcessStr));
        auto pid = SystemCall::Fork();
        if (pid < 0)
        {
            SystemCall::Write(static_cast<char const*>(ForkErrStr));
            SystemCall::Exit();
            return;
        }
        if (pid == 0) // child process
        {
            Loop(static_cast<char const*>(LoopParentStr));
        }
        else // parent process
        {
            Loop(static_cast<char const*>(LoopChildStr));
        }
    }
} // User namespace