#include "user_Program.h"

#include "user_SystemCall.h"

namespace
{
    /**
     * Delay for a certain number of cycles
     * 
     * @param aCount Number of cycles
     */
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
    void Process()
    {
        SystemCall::Write("User process\n\r");
        auto pid = SystemCall::Fork();
        if (pid < 0)
        {
            SystemCall::Write("Error during fork\n\r");
            SystemCall::Exit();
            return;
        }
        if (pid == 0) // child process
        {
            Loop("abcde");
        }
        else // parent process
        {
            Loop("12345");
        }
    }
} // User namespace