// TODO
// Right now this is a holding place for symbols the linker wants defined related to exception handling
// They don't really do anything, so if we actually throw exceptions, that will be a problem. But we
// don't yet, so it's ok.
//
// See: https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html
// 
// Unclear if that's what's being followed for AARCH64, but those are the symbols that clang wants.
// Will need to do more research later to see how to properly handle exceptions on ARM.

#include <cstdint>

namespace
{
    using ExitFunction = void (*)(void *apThis);

    enum _Unwind_Reason_Code
    {
        _URC_NO_REASON = 0,
        _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
        _URC_FATAL_PHASE2_ERROR = 2,
        _URC_FATAL_PHASE1_ERROR = 3,
        _URC_NORMAL_STOP = 4,
        _URC_END_OF_STACK = 5,
        _URC_HANDLER_FOUND = 6,
        _URC_INSTALL_CONTEXT = 7,
        _URC_CONTINUE_UNWIND = 8,
    };

    using _Unwind_Action = int;
    static constexpr _Unwind_Action _UA_SEARCH_PHASE = 1;
    static constexpr _Unwind_Action _UA_CLEANUP_PHASE = 2;
    static constexpr _Unwind_Action _UA_HANDLER_FRAME = 4;
    static constexpr _Unwind_Action _UA_FORCE_UNWIND = 8;

    struct _Unwind_Exception;
    struct _Unwind_Context;
    using _Unwind_Exception_Cleanup_Fn = void (*)(_Unwind_Reason_Code reason, _Unwind_Exception* exc);

    struct _Unwind_Exception
    {
        uint64_t exception_class;
        _Unwind_Exception_Cleanup_Fn exception_cleanup;
        uint64_t private_1;
        uint64_t private_2;
    };
}

extern "C"
{
    /**
     * Register a destructor function to be called by exit() or when a shared library is unloaded. When a shared library is
     * unloaded, apExitFunction is called with apThis as the parameter, and it's removed from the list to be called. On a
     * call to exit(), all remaining functions are called. Functions are always called in reverse order of registration.
     * 
     * @param apExitFunction Function to be called with apThis as its parameter
     * @param apThis Parameter to be passed to the function when it is called
     * @param apHandle Handle to the shared library associated with the function
     */
    void __cxa_atexit(ExitFunction apExitFunction, void* apThis, void* apHandle)
    {
        // TODO
    }

    /**
     * Called on initialization of the catch parameter. It increments the exception's handler count, puts the exception on
     * the stack of currently caught exceptions if it isn't already there (linking the exception to the previous top of the
     * stack), decrements the uncaught exception count, and returns the adjusted pointer to the exception object.
     * 
     * @param apExceptionObject The exception being caught
     * 
     * @return The adjusted exception object pointer
     */
    void* __cxa_begin_catch(void* apExceptionObject)
    {
        // TODO
        return apExceptionObject;
    }

    /**
     * The personality routine that provides translation between system unwind library and language-specific exception handling semantics
     * 
     * @param aVersion Version of the unwinding routine for detecting mismatches
     * @param aActions Processing to perform, as a bitmask
     * @param aExceptionClass 8-byte identifier specifying the type of exception. By convention, the high four bits are the vendor, and
     * the low four bits are the language.
     * @param apExceptionObject Pointer to the information needed for processing the exception
     * @param apContext Unwinder state information
     * 
     * @return How further unwind should happen, or any errors
     */
    _Unwind_Reason_Code __gxx_personality_v0(int /*aVersion*/, _Unwind_Action /*aActions*/, uint64_t /*aExceptionClass*/,
                                             _Unwind_Exception* /*apExceptionObject*/, _Unwind_Context* /*apContext*/)
    {
        // TODO
        return _URC_CONTINUE_UNWIND;
    }
}