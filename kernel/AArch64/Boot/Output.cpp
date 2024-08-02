// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include "Output.h"

#include <cstddef>
#include <cstring>

#include "../CPU.h"
#include "../../MemoryManager.h"

extern "C"
{
    // DO NOT ACCESS DIRECTLY
    // We can access these before the MMU is set up, so always go through the GetX() functions instead

    // from link.ld
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    extern char _output_buffer[];
    extern char _output_buffer_end[];
    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
}

namespace AArch64::Boot
{
    namespace
    {
        // DO NOT ACCESS DIRECTLY
        // We can access these before the MMU is set up, so always go through the GetX() functions instead

        // Kind of janky, but keep track of whether we've written anything so we can ensure that if they request the
        // buffer with no output, they get nothing
        auto AnyOutputWritten = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        // #TODO: Switch to function level static once we support __cxa_guard_acquire/__cxa_guard_release
        auto BufferOffset = 0ULL; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        /**
         * Obtain the buffer start pointer, adjusted for whether the MMU is on or not
         * 
         * @return The buffer start pointer
         */
        char* GetBufferStart()
        {
            return MemoryManager::AdjustKernelPtrForMMU(static_cast<char*>(_output_buffer));
        }

        /**
         * Obtain the buffer end pointer, adjusted for whether the MMU is on or not
         * 
         * @return The buffer end pointer
         */
        char* GetBufferEnd()
        {
            return MemoryManager::AdjustKernelPtrForMMU(static_cast<char*>(_output_buffer_end));
        }

        /**
         * Obtain the any output written bool, adjusted for whether the MMU is on or not
         * 
         * @return Reference to the output written bool
         */
        bool& GetAnyOutputWritten()
        {
            return *MemoryManager::AdjustKernelPtrForMMU(&AnyOutputWritten);
        }

        /**
         * Obtain the buffer offset, adjusted for whether the MMU is on or not
         * 
         * @return Reference to the buffer offset index
         */
        unsigned long long& GetBufferOffset()
        {
            return *MemoryManager::AdjustKernelPtrForMMU(&BufferOffset);
        }

        /**
         * Output text to our buffer
         * 
         * @param apMessage The message to output
         * @param aNewLine Whether to output a newline or not
         */
        void OutputText(char const* const apMessage, bool const aNewLine) // NOLINT(misc-no-recursion)
        {
            // Since we are called during the boot process, possibly before the MMU is set up, apMessage might be a
            // virtual address instead of a physical one, so we need to adjust the pointer for that
            auto const* const pfixedMessage = MemoryManager::AdjustKernelPtrForMMU(apMessage);

            auto* const pbufferStart = GetBufferStart();
            auto* const pbufferEnd = GetBufferEnd();
            auto& rbufferOffset = GetBufferOffset();

            GetAnyOutputWritten() = true;
            // #TODO: Switch to function level static once we support __cxa_guard_acquire/__cxa_guard_release
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            auto const BufferSizeCS = static_cast<std::size_t>(pbufferEnd - pbufferStart);

            auto const messageLen = strlen(pfixedMessage);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            auto const remainingLen = BufferSizeCS - rbufferOffset;
            
            if ((messageLen + 1) > remainingLen)
            {
                // specifically bash whatever is at the start before halting
                // #TODO: Switch to strcpy when we have it
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
                static constexpr char const bufferFullMsg[] = "PANIC: Output buffer full";
                // Intentionally do NOT include the null terminator so it's easier to see what's left of the buffer in
                // the debugger
                static constexpr auto bufferFullMsgLen = ((sizeof(bufferFullMsg) / sizeof(bufferFullMsg[0])) - 1);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
                memcpy(pbufferStart, "PANIC: Output buffer full", bufferFullMsgLen);

                CPU::Halt();
            }
            else
            {
                // NOLINTNEXTLINE(bugprone-not-null-terminated-result,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
                memcpy(pbufferStart + rbufferOffset, pfixedMessage, messageLen);
                rbufferOffset += messageLen;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                pbufferStart[rbufferOffset] = '\0';
                // do NOT increment the offset so any next write will bash the null

                if (aNewLine)
                {
                    OutputText("\r\n", false);
                }
            }
        }
    }

    void PanicImpl(char const* const apMessage)
    {
        OutputText("PANIC: ", false);
        OutputText(apMessage, true);
        // #TODO: Would be nice if we could trigger a breakpoint in some way
        CPU::Halt();
    }

    void OutputDebugImpl(char const* const apMessage)
    {
        OutputText(apMessage, true);
    }

    char const* GetOutputBuffer()
    {
        char* pbufferStart = GetBufferStart();
        if (!GetAnyOutputWritten())
        {
            *pbufferStart = 0;
        }
        return pbufferStart;
    }
}