#include "../PointerTypes.h"

#include <cstring>
#include "Framework.h"

namespace UnitTests::PointerTypes
{
    namespace
    {
        static_assert(PhysicalPtr{ 10 }.GetAddress() == 10, "Constructor didn't store address");
        static_assert(PhysicalPtr{ 10 }.Offset(15).GetAddress() == 25, "Unexpected result from positive offset");
        static_assert((PhysicalPtr{ 10 } == PhysicalPtr{ 10 }) && !(PhysicalPtr{ 10 } == PhysicalPtr{ 15 }), "Unexpected equality check result");
        static_assert(!(PhysicalPtr{ 10 } != PhysicalPtr{ 10 }) && (PhysicalPtr{ 10 } != PhysicalPtr{ 15 }), "Unexpected inequality check result");

        static_assert(VirtualPtr{ 10 }.GetAddress() == 10, "Constructor didn't store address");
        static_assert(VirtualPtr{ 10 }.Offset(15).GetAddress() == 25, "Unexpected result from positive offset");
        static_assert((VirtualPtr{ 10 } == VirtualPtr{ 10 }) && !(VirtualPtr{ 10 } == VirtualPtr{ 15 }), "Unexpected equality check result");
        static_assert(!(VirtualPtr{ 10 } != VirtualPtr{ 10 }) && (VirtualPtr{ 10 } != VirtualPtr{ 15 }), "Unexpected inequality check result");

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