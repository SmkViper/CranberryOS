// IMPORTANT: Code in this file should be very careful with accessing any global variables, as the MMU is not
// not initialized, and the linker maps everythin the kernel into the higher-half.

#include "Output.h"

#include <cstddef>
#include <cstring>

#include "MMU.h"
#include "../CPU.h"

extern "C"
{
    // DO NOT ACCESS DIRECTLY
    // Use GlobalNoMMU() instead

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
        // Use GlobalNoMMU() instead

        // Kind of janky, but keep track of whether we've written anything so we can ensure that if they request the
        // buffer with no output, they get nothing
        auto AnyOutputWrittenS = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        // #TODO: Switch to function level static once we support __cxa_guard_acquire/__cxa_guard_release
        auto BufferOffsetS = 0ULL; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        // Struct to give to the output functions so it knows how to access the globals
        struct OutputData
        {
            char* pBufferStart = nullptr;
            char* pBufferEnd = nullptr;
            decltype(AnyOutputWrittenS)* pAnyOutputWritten = nullptr;
            decltype(BufferOffsetS)* pBufferOffset = nullptr;
        };

        /**
         * Copies text to the given buffer, avoiding memcpy
         * 
         * @param apOutput Buffer to copy to
         * @param apText Text to copy
         * @param aCount Number of characters to copy
         */
        void CopyToBuffer(char* const apOutput, char const* const apText, size_t const aCount)
        {
            // #TODO: Compiler seems to be a bit too smart with optimizing memcpy and even just this loop before we
            // have the CPU and MMU fully set up. So we force this ugly volatile and manual loop here to ensure that
            // things are copied byte by byte until we can figure out what exactly needs to change
            [[maybe_unused]] volatile char deOptimize = 0;
            for (auto curIndex = 0U; curIndex < aCount; ++curIndex)
            {
                deOptimize = apText[curIndex]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                apOutput[curIndex] = apText[curIndex]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
        }

        /**
         * Output text to our buffer
         * 
         * @param apMessage The message to output
         * @param aNewLine Whether to output a newline or not
         * @param aOutput Pointers to the globals for output
         */
        // NOLINTNEXTLINE(misc-no-recursion)
        void OutputText(char const* const apMessage, bool const aNewLine, OutputData const& aOutput)
        {
            *aOutput.pAnyOutputWritten = true;
            // #TODO: Switch to function level static once we support __cxa_guard_acquire/__cxa_guard_release
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            auto const BufferSizeCS = static_cast<std::size_t>(aOutput.pBufferEnd - aOutput.pBufferStart);

            auto const messageLen = strlen(apMessage);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            auto const remainingLen = BufferSizeCS - *aOutput.pBufferOffset;
            
            if ((messageLen + 1) > remainingLen)
            {
                // specifically bash whatever is at the start before halting
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
                static constexpr char const bufferFullMsg[] = "PANIC: Output buffer full";
                // Intentionally do NOT include the null terminator so it's easier to see what's left of the buffer in
                // the debugger
                static constexpr auto bufferFullMsgLen = ((sizeof(bufferFullMsg) / sizeof(bufferFullMsg[0])) - 1);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
                CopyToBuffer(aOutput.pBufferStart, "PANIC: Output buffer full", bufferFullMsgLen);

                CPU::Halt();
            }
            else
            {
                // NOLINTNEXTLINE(bugprone-not-null-terminated-result,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
                CopyToBuffer(aOutput.pBufferStart + *aOutput.pBufferOffset, apMessage, messageLen);
                *aOutput.pBufferOffset += messageLen;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                aOutput.pBufferStart[*aOutput.pBufferOffset] = '\0';
                // do NOT increment the offset so any next write will bash the null

                if (aNewLine)
                {
                    OutputText("\r\n", false, aOutput);
                }
            }
        }
    }

    void PanicNoMMU(char const* const apMessage)
    {
        // pointer we get might be a virtual address from the linker, so fix it up first
        auto const* const pfixedMessage = GlobalNoMMU(apMessage);

        auto data = OutputData{
            .pBufferStart = static_cast<char*>(GlobalNoMMU(_output_buffer)),
            .pBufferEnd = static_cast<char*>(GlobalNoMMU(_output_buffer_end)),
            .pAnyOutputWritten = &GlobalNoMMU(AnyOutputWrittenS),
            .pBufferOffset = &GlobalNoMMU(BufferOffsetS)
        };

        OutputText("PANIC: ", false, data);
        OutputText(pfixedMessage, true, data);
        // #TODO: Would be nice if we could trigger a breakpoint in some way
        CPU::Halt();
    }

    void PanicMMU(char const* const apMessage)
    {
        auto data = OutputData{
            .pBufferStart = static_cast<char*>(_output_buffer),
            .pBufferEnd = static_cast<char*>(_output_buffer_end),
            .pAnyOutputWritten = &AnyOutputWrittenS,
            .pBufferOffset = &BufferOffsetS
        };

        OutputText("PANIC: ", false, data);
        OutputText(apMessage, true, data);
        // #TODO: Would be nice if we could trigger a breakpoint in some way
        CPU::Halt();
    }

    void OutputDebugNoMMU(char const* const apMessage)
    {
        // pointer we get might be a virtual address from the linker, so fix it up first
        auto const* const pfixedMessage = GlobalNoMMU(apMessage);

        auto data = OutputData{
            .pBufferStart = static_cast<char*>(GlobalNoMMU(_output_buffer)),
            .pBufferEnd = static_cast<char*>(GlobalNoMMU(_output_buffer_end)),
            .pAnyOutputWritten = &GlobalNoMMU(AnyOutputWrittenS),
            .pBufferOffset = &GlobalNoMMU(BufferOffsetS)
        };

        OutputText(pfixedMessage, true, data);
    }

    void OutputDebugMMU(char const* const apMessage)
    {
        auto data = OutputData{
            .pBufferStart = static_cast<char*>(_output_buffer),
            .pBufferEnd = static_cast<char*>(_output_buffer_end),
            .pAnyOutputWritten = &AnyOutputWrittenS,
            .pBufferOffset = &BufferOffsetS
        };

        OutputText(apMessage, true, data);
    }

    char const* GetOutputBuffer()
    {
        auto* pbufferStart = static_cast<char*>(_output_buffer);
        if (!AnyOutputWrittenS)
        {
            *pbufferStart = 0;
        }
        return pbufferStart;
    }
}