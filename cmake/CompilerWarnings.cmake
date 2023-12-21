function(CranberryOS_SetProjectWarnings aProject aWarningsAsErrors)
    set(clangWarnings
        # TODO: Re-add when we fix the warnings
        #-Wall
        -Wextra
        -Wshadow                # don't want variables shadowing
        -Wnon-virtual-dtor      # don't want classes with vtables not having virtual destructors
        -Wold-style-cast        # no c-style casts
        -Wcast-align            # casts that may cause performance issues
        -Wunused                # make sure nothing is unused
        -Woverloaded-virtual    # a virtual is overloaded (not overridden)
        -Wpedantic              # non-standard C++
        -Wconversion            # type conversions that lose data
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion      # implicit float -> double conversion
        -Wformat=2              # issues around formatting functions (i.e. printf)
        -Wimplicit-fallthrough  # statements that fallthrough in case blocks
    )

    if(aWarningsAsErrors)
        message(TRACE "Warnings treated as errors")
        list(APPEND clangWarnings -Werror)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(projectWarningsCXX ${clangWarnings})
    else()
        message(AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILERID}'")
    endif()

    set(projectWarningsC "${projectWarningsCXX}")

    target_compile_options(
        ${aProject}
        INTERFACE
            $<$<COMPILE_LANGUAGE:CXX>:${projectWarningsCXX}>
            $<$<COMPILE_LANGUAGE:C>:${projectWarningsC}>
    )
endfunction()