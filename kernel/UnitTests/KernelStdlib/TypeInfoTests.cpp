#include "TypeInfoTests.h"

#include <cstring>
#include <typeinfo>

#include "../Framework.h"

namespace UnitTests::KernelStdlib::TypeInfo
{
    namespace
    {
        void TestTypeInfo()
        {
            EmitTestSkipResult("type_info tests");
            // Disabling RTTI tests for now because we've turned off rtti in the compilation options. I'd like to
            // support RTTI at some point (and exceptions) but attempting to turn it on with -frtti (or just by having
            // no command line arguments related to RTTI) just results in "undefined reference to typeinfo" errors from
            // the linker. I'm guessing one of our baremetal arguments is conflicting with it and forcing rtti data
            // generation off - or perhaps something is wrong with our implementation

            /*
            auto const& intTypeID1 = typeid(int);
            auto const& intTypeID2 = typeid(int);
            auto const& floatTypeID = typeid(float);

            EmitTestResult(intTypeID1 == intTypeID2, "type_info operator==");
            EmitTestResult(intTypeID1 != floatTypeID, "type_info operator!=");

            // before() is allowed to change run-to-run, so we just make sure the two types before() agrees
            if (intTypeID1.before(floatTypeID))
            {
                EmitTestResult(!floatTypeID.before(intTypeID1), "type_info before()");
            }
            else
            {
                EmitTestResult(floatTypeID.before(intTypeID1), "type_info before()");
            }

            // name() has no guarantees, but for clang (which we're using) it produces the Itanium C++ ABI mangled name
            // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-type
            EmitTestResult(strcmp(intTypeID1.name(), "i") == 0, "type_info name()");
            */
        }
    }

    void Run()
    {
        TestTypeInfo();
    }
}