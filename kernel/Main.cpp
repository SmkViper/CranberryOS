#include "MiniUart.h"
#include "Print.h"
#include "UnitTests.h"
#include "Utils.h"

namespace
{
    using StaticInitFunction = void (*)();
    using StaticFiniFunction = void (*)();
}

extern "C"
{
    // Defined by the linker to point at the start/end of the init/fini arrays
    extern StaticInitFunction __init_start;
    extern StaticInitFunction __init_end;
    extern StaticFiniFunction __fini_start;
    extern StaticFiniFunction __fini_end;
}

namespace
{
    /**
     * Calls all static constructors in our .init_array section
     */
    void CallStaticConstructors()
    {
        for (auto pcurFunc = &__init_start; pcurFunc != &__init_end; ++pcurFunc)
        {
            (*pcurFunc)();
        }
    }

    /**
     * Calls all static destructors in our .fini_array section
     */
    void CallStaticDestructors()
    {
        for (auto pcurFunc = &__fini_start; pcurFunc != &__fini_end; ++pcurFunc)
        {
            (*pcurFunc)();
        }
    }
}

// Called from assembly, so don't mangle the name
extern "C"
{
    /**
     * Kernel entry point
     */
    void kmain()
    {
        CallStaticConstructors();

        MiniUART::Init();
        UnitTests::Run();

        char outputBuffer[256];

        Print::FormatToBuffer(outputBuffer, "Test format with no args\r\n");
        MiniUART::SendString(outputBuffer);

        const char* pdecayedString = "decayed string";
        Print::FormatToMiniUART("Test with 3 values - integer {}, {}, and {}\r\n", 204u, "string", pdecayedString);

        MiniUART::SendString("Hello, World! Type 'q' to \"exit\"\r\n");

        bool done = false;
        while(!done)
        {
            const auto valueEntered = MiniUART::Receive();
            MiniUART::Send(valueEntered); // always echo it to the user
            done = (valueEntered == 'q');
        }

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        CallStaticDestructors();

        UnitTests::RunPostStaticDestructors();
    }
}