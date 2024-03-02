#include "../PointerTypes.h"

#include <cstring>
#include "../Print.h"
#include "Framework.h"

namespace UnitTests::PointerTypes
{
    namespace
    {
        // Disable some lints for this file since they don't make sense here
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

        static_assert(PhysicalPtr{}.GetAddress() == 0, "Constructor didn't make null pointer");
        static_assert(PhysicalPtr{ 10 }.GetAddress() == 10, "Constructor didn't store address");
        static_assert(PhysicalPtr{ 10 }.Offset(15).GetAddress() == 25, "Unexpected result from positive offset");
        static_assert((PhysicalPtr{ 10 } == PhysicalPtr{ 10 }) && !(PhysicalPtr{ 10 } == PhysicalPtr{ 15 }), "Unexpected equality check result");
        static_assert(!(PhysicalPtr{ 10 } != PhysicalPtr{ 10 }) && (PhysicalPtr{ 10 } != PhysicalPtr{ 15 }), "Unexpected inequality check result");
        static_assert(!(PhysicalPtr{ 10 } < PhysicalPtr{ 10 }) && (PhysicalPtr{ 10 } < PhysicalPtr{ 15 }) && !(PhysicalPtr{ 15 } < PhysicalPtr{ 10 }), "Unexpected less than check result");
        static_assert(!(PhysicalPtr{ 10 } > PhysicalPtr{ 10 }) && !(PhysicalPtr{ 10 } > PhysicalPtr{ 15 }) && (PhysicalPtr{ 15 } > PhysicalPtr{ 10 }), "Unexpected greater than check result");
        static_assert((PhysicalPtr{ 10 } <= PhysicalPtr{ 10 }) && (PhysicalPtr{ 10 } <= PhysicalPtr{ 15 }) && !(PhysicalPtr{ 15 } <= PhysicalPtr{ 10 }), "Unexpected less or equal check result");
        static_assert((PhysicalPtr{ 10 } >= PhysicalPtr{ 10 }) && !(PhysicalPtr{ 10 } >= PhysicalPtr{ 15 }) && (PhysicalPtr{ 15 } >= PhysicalPtr{ 10 }), "Unexpected greater or equal check result");

        static_assert(VirtualPtr{}.GetAddress() == 0, "Constructor didn't make null pointer");
        static_assert(VirtualPtr{ 10 }.GetAddress() == 10, "Constructor didn't store address");
        static_assert(VirtualPtr{ 10 }.Offset(15).GetAddress() == 25, "Unexpected result from positive offset");
        static_assert((VirtualPtr{ 10 } == VirtualPtr{ 10 }) && !(VirtualPtr{ 10 } == VirtualPtr{ 15 }), "Unexpected equality check result");
        static_assert(!(VirtualPtr{ 10 } != VirtualPtr{ 10 }) && (VirtualPtr{ 10 } != VirtualPtr{ 15 }), "Unexpected inequality check result");
        static_assert(!(VirtualPtr{ 10 } < VirtualPtr{ 10 }) && (VirtualPtr{ 10 } < VirtualPtr{ 15 }) && !(VirtualPtr{ 15 } < VirtualPtr{ 10 }), "Unexpected less than check result");
        static_assert(!(VirtualPtr{ 10 } > VirtualPtr{ 10 }) && !(VirtualPtr{ 10 } > VirtualPtr{ 15 }) && (VirtualPtr{ 15 } > VirtualPtr{ 10 }), "Unexpected greater than check result");
        static_assert((VirtualPtr{ 10 } <= VirtualPtr{ 10 }) && (VirtualPtr{ 10 } <= VirtualPtr{ 15 }) && !(VirtualPtr{ 15 } <= VirtualPtr{ 10 }), "Unexpected less or equal check result");
        static_assert((VirtualPtr{ 10 } >= VirtualPtr{ 10 }) && !(VirtualPtr{ 10 } >= VirtualPtr{ 15 }) && (VirtualPtr{ 15 } >= VirtualPtr{ 10 }), "Unexpected greater or equal check result");

        /**
         * Testing PhysicalPtr formatting
         */
        void PhysicalPtrPrintTest()
        {
            char buffer[256]; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            Print::FormatToBuffer(buffer, "{}", PhysicalPtr{ 0xFF });
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            EmitTestResult(strcmp(buffer, "P0xff") == 0, "PhysicalPtr print format");
        }

        /**
         * Testing VirtualPtr formatting
         */
        void VirtualPtrPrintTest()
        {
            char buffer[256]; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            Print::FormatToBuffer(buffer, "{}", VirtualPtr{ 0xFF });
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            EmitTestResult(strcmp(buffer, "V0xff") == 0, "VirtualPtr print format");
        }

        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    }

    void Run()
    {
        PhysicalPtrPrintTest();
        VirtualPtrPrintTest();
    }
}