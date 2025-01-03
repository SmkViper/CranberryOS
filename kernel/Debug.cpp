#include "Debug.h"

#include "AArch64/Boot/Output.h"

namespace Debug
{
    namespace
    {
        // We default initialize these to the boot level implementations (that require a MMU)
        // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
        PanicFn pCurPanicFunction = AArch64::Boot::PanicMMU;
        DebugOutFn pCurDebugOutFunction = AArch64::Boot::OutputDebugMMU;
        // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
    }

    void SetPanicFunction(PanicFn apPanicFunction)
    {
        pCurPanicFunction = apPanicFunction;
    }

    void SetDebugOutFunction(DebugOutFn apDebugOutFunction)
    {
        pCurDebugOutFunction = apDebugOutFunction;
    }

    void Panic(char const* const apMessage)
    {
        pCurPanicFunction(apMessage);
    }

    void OutputDebug(char const* const apMessage)
    {
        pCurDebugOutFunction(apMessage);
    }
}