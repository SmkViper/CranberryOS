#include "user_Program.h"

#include <cstdint>
#include "user_SystemCall.h"

namespace
{
    // We need to force all our strings into the .rodata.user segment so the kernel code that sets up the user process
    // successfully grabs them. Otherwise we'll trigger a memory exception when trying to access data in kernel code
    // #TODO: Not sure why this is needed for when LTO is on, but not when it is off. Maybe it's consolidating strings
    // that match across all compilation units, and therefore losing the section that these files have?
    __attribute__((section(".rodata.user")))
    static const char UserProcessStr[] = "User process\r\n";
    __attribute__((section(".rodata.user")))
    static const char ForkErrStr[] = "Error during fork\r\n";
    __attribute__((section(".rodata.user")))
    static const char LoopParentStr[] = "abcde";
    __attribute__((section(".rodata.user")))
    static const char LoopChildStr[] = "12345";

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
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
                SystemCall::Write(buffer);
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
        SystemCall::Write(UserProcessStr);
        auto pid = SystemCall::Fork();
        if (pid < 0)
        {
            SystemCall::Write(ForkErrStr);
            SystemCall::Exit();
            return;
        }
        if (pid == 0) // child process
        {
            Loop(LoopParentStr);
        }
        else // parent process
        {
            Loop(LoopChildStr);
        }
    }
} // User namespace