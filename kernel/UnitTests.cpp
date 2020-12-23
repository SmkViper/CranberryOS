#include "UnitTests.h"

#include "MiniUart.h"
#include "Utils.h"

namespace
{
    /**
     * Output a test failure message to MiniUART
     * 
     * @param apMessage The message to output along with the FAIL tag
     */
    void TestFailed(const char* const apMessage)
    {
        // We don't have printf yet, so this is a bit wonky
        MiniUART::Send('[');
        MiniUART::SendString("\x1b[31m"); // Set red color
        MiniUART::SendString("FAIL");
        MiniUART::SendString("\x1b[m"); // Return to defaults
        MiniUART::Send(']');
        MiniUART::SendString(apMessage);
        MiniUART::SendString("\r\n");
    }

    /**
     * Output a test passed message to MiniUART
     * 
     * @param apMessage The message to output along with the PASS tag
     */
    void TestPassed(const char* const apMessage)
    {
        // We don't have printf yet, so this is a bit wonky
        MiniUART::Send('[');
        MiniUART::SendString("\x1b[32m"); // Set green color
        MiniUART::SendString("PASS");
        MiniUART::SendString("\x1b[m"); // Return to defaults
        MiniUART::Send(']');
        MiniUART::SendString(apMessage);
        MiniUART::SendString("\r\n");
    }

    /**
     * Tiny helper to emit pass or failure based on a result bool
     * 
     * @param aResult The result of the test
     * @param apMessage The message to emit
     */
    void EmitTestResult(bool aResult, const char* apMessage)
    {
        if (aResult)
        {
            TestPassed(apMessage);
        }
        else
        {
            TestFailed(apMessage);
        }
    }

    // Using a non-zero, non-one magic numbers here to lower the chance that we're reading garbage memory and passing
    constexpr int StaticObjectInitialized = 10;
    constexpr int StaticObjectDestructed = -10;

    int StaticFunctionTarget = 0;

    /**
     * Ensure C-style "constructor" is called (GCC extension)
     */
    void __attribute__((constructor)) StaticConstructor()
    {
        StaticFunctionTarget = StaticObjectInitialized;
    }

    /**
     * Ensure C-style "destructor" is called (GCC extension)
     */
    void __attribute__((destructor)) StaticDestructor()
    {
        StaticFunctionTarget = StaticObjectDestructed;
    }

    /**
     * Test to ensure static C++ constructors are called
     */
    void StaticCConstructorTest()
    {
        EmitTestResult(StaticFunctionTarget == StaticObjectInitialized, "Static C function construction");
    }

    /**
     * Test to ensure static C++ destructors are called
     */
    void StaticCDestructorTest()
    {
        EmitTestResult(StaticFunctionTarget == StaticObjectDestructed, "Static C function destruction");
    }

    // Dummy test object and static variable to test static init/shutdown
    int StaticObjectTarget = 0;
    struct StaticConstructorDestructorTestObj
    {
        StaticConstructorDestructorTestObj()
        {
            StaticObjectTarget = StaticObjectInitialized;
        }

        ~StaticConstructorDestructorTestObj()
        {
            StaticObjectTarget = StaticObjectDestructed;
        }
    };

    StaticConstructorDestructorTestObj GlobalObject;

    /**
     * Test to ensure static C++ constructors are called
     */
    void StaticCppConstructorTest()
    {
        EmitTestResult(StaticObjectTarget == StaticObjectInitialized, "Static C++ construction");
    }

    /**
     * Test to ensure static C++ destructors are called
     */
    void StaticCppDestructorTest()
    {
        // TODO
        // Clang seems to assume the destructor has no observable side effects, and so does not add
        // it to the .fini_array, causing this test to fail. Need to find a way to subvert clang's
        // analysis (printing to MiniUART and volatile access seem to not be good enough)
        EmitTestResult(StaticObjectTarget == StaticObjectDestructed, "Static C++ destruction");
    }

    /**
     * Test to make sure we're running at the expected exception level
     */
    void ExceptionLevelTest()
    {
        EmitTestResult(CPU::GetExceptionLevel() == 1, "Exception level");
    }
}

namespace UnitTests
{
    void Run()
    {
        StaticCConstructorTest();
        StaticCppConstructorTest();
        ExceptionLevelTest();
    }

    void RunPostStaticDestructors()
    {
        StaticCDestructorTest();
        StaticCppDestructorTest();
    }
}