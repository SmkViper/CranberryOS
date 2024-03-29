#include "Framework.h"

#include <cstddef>
#include <cstdint>

#include "../Print.h"
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
#include "MemoryManagerTests.h"
#include "PointerTypesTests.h"
#include "PrintTests.h"
#include "UtilsTests.h"

namespace UnitTests
{
    namespace
    {
        unsigned TestsPassing = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        unsigned TestsFailing = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        unsigned TestsSkipped = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        constexpr unsigned GreenColor = 32;
        constexpr unsigned RedColor = 31;
        constexpr unsigned YellowColor = 33;
        constexpr std::size_t HeaderBufferSize = 32U;

        /**
         * Formats a colored string for terminal output
         * 
         * @param arBuffer Buffer to output to
         * @param apString The string to output
         * @param aColor The color the string should be (terminal escape sequence color)
         */
        template<std::size_t BufferSize>
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
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

        int StaticFunctionTarget = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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
        int StaticObjectTarget = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        struct StaticConstructorDestructorTestObj
        {
            StaticConstructorDestructorTestObj() noexcept
            {
                StaticObjectTarget = StaticObjectInitialized;
            }

            ~StaticConstructorDestructorTestObj()
            {
                StaticObjectTarget = StaticObjectDestructed;
            }

            StaticConstructorDestructorTestObj(StaticConstructorDestructorTestObj const&) = default;
            StaticConstructorDestructorTestObj(StaticConstructorDestructorTestObj&&) = default;
            StaticConstructorDestructorTestObj& operator=(StaticConstructorDestructorTestObj const&) = default;
            StaticConstructorDestructorTestObj& operator=(StaticConstructorDestructorTestObj&&) = default;
        };

        StaticConstructorDestructorTestObj GlobalObject; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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
            // #TODO: Clang seems to assume the destructor has no observable side effects, and so does not add it to
            // the .fini_array, causing this test to fail. Need to find a way to subvert clang's analysis (printing to
            // MiniUART and volatile access seem to not be good enough)
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
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            char passFailMessage[HeaderBufferSize];
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
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            char skipMessage[HeaderBufferSize];
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

        // Devices/* not tested as right now they're just constexpr values
        // #TODO: Devices/DeviceTree.h/cpp untested

        // No runtime tests for bit
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

        // #TODO: Exceptions.cpp untested (currently just unimplemented stubs)
        // #TODO: ExceptionVectorHandlers.h/cpp/S untested (not sure if testable)
        // #TODO: IRQ.h/S untested (likely untestable)
        MemoryManager::Run();
        PointerTypes::Run();
        // #TODO: MiniUart.h/cpp untested (likely untestable - though basically tested due to all our UART output)
        Print::Run();
        // #TODO: Scheduler.h/cpp/S untested (not sure if testable, other than our running user apps)
        // #TODO: SystemCall.cpp untested (not sure if testable, other than our running user apps)
        // #TODO: TaskStructs.h untested (currently just contains POD types)
        // #TODO: Timer.h/cpp untested (not sure if testable, as testing might disrupt OS behavior)
        // #TODO: TypeInfo.cpp untested (currently just contains types filled by the compiler)
        Utils::Run();

        // Build a quick reference output at the end
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
        char status[HeaderBufferSize];
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