#include <bit>

namespace UnitTests::KernelStdlib::TypeTraits
{
    namespace
    {
        ///////////////////////////////////////////////////////////////////////
        // std::bit_cast
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::bit_cast<int>(1.0f) == 0x3f80'0000, "Unexpected result from bit_cast");
        // #TODO: Probably more tests we could do here
        
        // #TODO: Need to find a way to test that bit_cast refuses to compile on types it can't work with
    }
}