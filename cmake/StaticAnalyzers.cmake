macro(CranberryOS_EnableClangTidy aProject aWarningsAsErrors)
    find_program(CLANGTIDY clang-tidy)
    if(CLANGTIDY)
        # #TODO: add "-system-headers" under header filter and reconfigure/rebuild once we fix header issues
        set(CLANG_TIDY_OPTIONS
            ${CLANGTIDY}
            -header-filter=.*
            -extra-arg=-Wno-unknown-warning-option
            -extra-arg=-Wno-ignored-optimization-argument
            -extra-arg=-Wno-unused-command-line-argument
            -p
            )
        
        # set the C++ standard version
        if(NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
            set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS} -extra-arg=-std=c++${CMAKE_CXX_STANDARD})
        endif()

        # set warnings as errors
        if(${aWarningsAsErrors})
            list(APPEND CLANG_TIDY_OPTIONS -warnings-as-errors=*)
        endif()
        message("Also setting clang-tidy globally")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_OPTIONS})
    else()
        message(${WARNING_MESSAGE} "clang-tidy requested but executable not found")
    endif()
endmacro()
