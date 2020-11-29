#include "MiniUart.h"

// Called from assembly, so don't mangle the name
extern "C"
{
    /**
     * Kernel entry point
     */
    void kmain(void)
    {
        MiniUART::Init();
        MiniUART::SendString("Hello, World!\r\n");

        // Don't ever exit, since there's nothing to exit to
        while(true)
        {
            MiniUART::Send(MiniUART::Receive());
        }
    }
}