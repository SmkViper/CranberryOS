#include "Framework.h"

#include <cstdint>

#include "AArch64/CPUTests.h"
#include "AArch64/MemoryDescriptorTests.h"
#include "AArch64/MemoryPageTablesTests.h"
#include "AArch64/SystemRegistersTests.h"
#include "KernelStdlib/BitsetTests.h"
#include "KernelStdlib/CStringTests.h"
#include "KernelStdlib/ExceptionTests.h"
#include "KernelStdlib/NewTests.h"
#include "KernelStdlib/TypeInfoTests.h"
#include "KernelStdlib/UtilityTests.h"
#include "Framework.h"
#include "MemoryManagerTests.h"
#include "PrintTests.h"
#include "UtilsTests.h"

namespace UnitTests
{
    namespace
    {
        unsigned TestsPassing = 0;
        unsigned TestsFailing = 0;
        unsigned TestsSkipped = 0;

        constexpr unsigned GreenColor = 32;
        constexpr unsigned RedColor = 31;
        constexpr unsigned YellowColor = 33;

        /**
         * Formats a colored string for terminal output
         * 
         * @param arBuffer Buffer to output to
         * @param apString The string to output
         * @param aColor The color the string should be (terminal escape sequence color)
         */
        template<std::size_t BufferSize>
        void FormatColoredString(char (&arBuffer)[BufferSize], char const* const apString, uint32_t const aColor)
        {
            ::Print::FormatToBuffer(arBuffer, "\x1b[{}m{}\x1b[m", aColor, apString);
        }

        ///////////////////////////////////////////////////////////////////////////
        // A variety of tests that aren't associated with any file
        ///////////////////////////////////////////////////////////////////////////

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
    }

    namespace Details
    {
        /**
         * Emits pass or failure based on a result bool
         * 
         * @param aResult The result of the test
         * @param apMessage The message to emit
         */
        void EmitTestResultImpl(const bool aResult, const char* const apMessage)
        {
            char passFailMessage[32];
            if (aResult)
            {
                TestsPassing += 1;
                FormatColoredString(passFailMessage, "PASS", GreenColor);
            }
            else
            {
                TestsFailing += 1;
                FormatColoredString(passFailMessage, "FAIL", RedColor);
            }
            ::Print::FormatToMiniUART("[{}] {}\r\n", passFailMessage, apMessage);
        }

        /**
         * Emits a skip message
         * 
         * @param apMessage The message to emit
         */
        void EmitTestSkipResultImpl(char const* const apMessage)
        {
            TestsSkipped += 1;
            char skipMessage[32];
            FormatColoredString(skipMessage, "SKIP", YellowColor);
            ::Print::FormatToMiniUART("[{}] {}\r\n", skipMessage, apMessage);
        }
    }

    void Run()
    {
        StaticCConstructorTest();
        StaticCppConstructorTest();

        AArch64::CPU::Run();
        AArch64::MemoryDescriptor::Run();
        AArch64::MemoryPageTables::Run();
        AArch64::SystemRegisters::Run();

        KernelStdlib::Bitset::Run();
        // No runtime tests for climits
        // No runtime tests for cstddef
        // No runtime tests for cstdint
        KernelStdlib::CString::Run();
        KernelStdlib::Exception::Run();
        KernelStdlib::New::Run();
        KernelStdlib::TypeInfo::Run();
        // No runtime tests for type_traits
        KernelStdlib::Utility::Run();

        MemoryManager::Run();
        Print::Run();
        Utils::Run();

        // Build a quick reference output at the end
        char status[32];
        if (TestsFailing != 0)
        {
            FormatColoredString(status, "FAIL", RedColor);
        }
        else if (TestsSkipped != 0)
        {
            FormatColoredString(status, "PASS", YellowColor);
        }
        else
        {
            FormatColoredString(status, "PASS", GreenColor);
        }
        ::Print::FormatToMiniUART("[{}] Passing: {} Failed: {} Skipped: {}\r\n", status, TestsPassing, TestsFailing, TestsSkipped);
    }

    void RunPostStaticDestructors()
    {
        StaticCDestructorTest();
        StaticCppDestructorTest();
    }
}