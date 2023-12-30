#include "../PointerTypes.h"

#include <cstring>
#include "Framework.h"

namespace UnitTests::PointerTypes
{
    namespace
    {
        static_assert(PhysicalPtr{ 10 }.GetAddress() == 10, "Constructor didn't store address");
        static_assert(VirtualPtr{ 10 }.GetAddress() == 10, "Constructor didn't store address");

        /**
         * Testing PhysicalPtr formatting
         */
        void PhysicalPtrPrintTest()
        {
            char buffer[256];
            Print::FormatToBuffer(buffer, "{}", PhysicalPtr{ 0xFF });
            EmitTestResult(strcmp(buffer, "P0xff") == 0, "PhysicalPtr print format");
        }

        /**
         * Testing VirtualPtr formatting
         */
        void VirtualPtrPrintTest()
        {
            char buffer[256];
            Print::FormatToBuffer(buffer, "{}", VirtualPtr{ 0xFF });
            EmitTestResult(strcmp(buffer, "V0xff") == 0, "VirtualPtr print format");
        }
    }

    void Run()
    {
        PhysicalPtrPrintTest();
        VirtualPtrPrintTest();
    }
}