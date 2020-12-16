#include "MiniUart.h"

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

    // Dummy test object and static variable to test static init/shutdown
    struct SomeObject
    {
        // Using a non-zero, non-one magic number here to lower the chance that we're reading garbage memory and passing
        // we can't send a string out over UART at this point because it isn't initialized. There's a bit of a catch-22
        // here. Ideally, we'd init UART before static initialization so our static variables can use it for debug output,
        // but in the future, that init process may use static variables/objects that would require construction. Once we
        // have C++20 however, we can likely use consteval on the values it uses to make sure they are built into the 
        // binary without needing their constructors called
        SomeObject() : InitMagic{10} {}
        ~SomeObject() { MiniUART::SendString("SomeObject::~SomeObject()\r\n"); }

        int InitMagic;
    };

    SomeObject GlobalObject;
}

// Called from assembly, so don't mangle the name
extern "C"
{
    /**
     * Kernel entry point
     */
    void kmain(void)
    {
        CallStaticConstructors();

        MiniUART::Init();
        MiniUART::SendString("Hello, World! Type 'q' to \"exit\"\r\n");

        if (GlobalObject.InitMagic == 10)
        {
            MiniUART::SendString("SomeObject::SomeObject() was called\r\n");
        }
        else
        {
            MiniUART::SendString("SomeObject::SomeObject() was NOT called\r\n");
        }
        
        bool done = false;
        while(!done)
        {
            const auto valueEntered = MiniUART::Receive();
            MiniUART::Send(valueEntered); // always echo it to the user
            done = (valueEntered == 'q');
        }

        MiniUART::SendString("\r\nExiting... (sending CPU into an infinite loop)\r\n");

        // TODO
        // For now this doesn't seem to do anything, as for some reason our destructor is not being added to
        // .fini_array. Will eventually have to figure out why.
        CallStaticDestructors();
    }
}