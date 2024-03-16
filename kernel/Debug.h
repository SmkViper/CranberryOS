#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

namespace Debug
{
    using PanicFn = void(*)(char const* apMessage);
    using DebugOutFn = void(*)(char const* apMessage);

    /**
     * Switch the panic output function
     * 
     * @param apPanicFunction The panic function
     */
    void SetPanicFunction(PanicFn apPanicFunction);

    /**
     * Switch the debug output function
     * 
     * @param apDebugOutFunction The debug output function
     */
    void SetDebugOutFunction(DebugOutFn apDebugOutFunction);

    /**
     * Output a panic message and halt the CPU
     * 
     * @param apMessage Message to output
     */
    void Panic(char const* apMessage);

    /**
     * Output a debug message
     * 
     * @param apMessage Message to output
     */
    void OutputDebug(char const* apMessage);
}

#endif // KERNEL_DEBUG_H