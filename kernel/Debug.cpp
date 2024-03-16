#include "Debug.h"

#include "AArch64/Boot/Output.h"

namespace Debug
{
    namespace
    {
        // We default initialize these to the boot level implementations
        // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
        PanicFn pCurPanicFunction = AArch64::Boot::PanicImpl;
        DebugOutFn pCurDebugOutFunction = AArch64::Boot::OutputDebugImpl;
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

    void Panic(char const* apMessage)
    {
        pCurPanicFunction(apMessage);
    }

    void OutputDebug(char const* apMessage)
    {
        pCurDebugOutFunction(apMessage);
    }
}