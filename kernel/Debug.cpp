#include "Debug.h"

#include "AArch64/Boot/Output.h"

#include "MemoryManager.h"

namespace Debug
{
    namespace
    {
        // DO NOT ACCESS DIRECTLY
        // We can access these before the MMU is set up, so always go through the GetX() functions instead

        // We default initialize these to the boot level implementations
        // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
        PanicFn pCurPanicFunction = AArch64::Boot::PanicImpl;
        DebugOutFn pCurDebugOutFunction = AArch64::Boot::OutputDebugImpl;
        // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

        // #TODO: Would like a better way to do this without having to check the MMU state twice per output call. Maybe
        // we can assume the function pointer is always correct for the MMU state? Or we could have the pointer updated
        // when it is set, and then have the boot process tell us to refresh our pointer. That would get the MMU
        // enable flag checks to one per call

        /**
         * Get a reference to the panic function pointer static, adjusted for MMU state
         * 
         * @return The panic function pointer static
         */
        PanicFn& GetPanicFunctionPtrStatic()
        {
            return *MemoryManager::AdjustKernelPtrForMMU(&pCurPanicFunction);
        }

        /**
         * Get the panic function pointer, adjusted for MMU state
         * 
         * @return The panic function pointer
         */
        PanicFn GetPanicFunction()
        {
            return MemoryManager::AdjustKernelPtrForMMU(GetPanicFunctionPtrStatic());
        }

        /**
         * Get a reference to the debug output function pointer static, adjusted for MMU state
         * 
         * @return The debug output function pointer static
         */
        DebugOutFn& GetDebugOutFunctionPtrStatic()
        {
            return *MemoryManager::AdjustKernelPtrForMMU(&pCurDebugOutFunction);
        }

        /**
         * Get the debug output function poitner, adjusted for MMU state
         * 
         * @return The debug output function pointer
         */
        PanicFn GetDebugOutFunction()
        {
            return MemoryManager::AdjustKernelPtrForMMU(GetDebugOutFunctionPtrStatic());
        }
    }

    void SetPanicFunction(PanicFn apPanicFunction)
    {
        GetPanicFunctionPtrStatic() = apPanicFunction;
    }

    void SetDebugOutFunction(DebugOutFn apDebugOutFunction)
    {
        GetDebugOutFunctionPtrStatic() = apDebugOutFunction;
    }

    void Panic(char const* apMessage)
    {
        GetPanicFunction()(apMessage);
    }

    void OutputDebug(char const* apMessage)
    {
        GetDebugOutFunction()(apMessage);
    }
}