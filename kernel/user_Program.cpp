#include "user_Program.h"

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
    void Delay(uint64_t aCount)
    {
        while (aCount--)
        {
            // make sure the compiler doesn't optimize out the decrementing so that this is
            // an actual cycle count delay
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
        char buffer[] = {'\0', '\0'};
        while (true)
        {
            for (auto curIndex = 0u; apStr[curIndex] != '\0'; ++curIndex)
            {
                buffer[0] = apStr[curIndex];
                SystemCall::Write(buffer);
                Delay(100000);
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