include(CheckCXXCompilerFlag)

macro(CranberryOS_SetupOptions)
    option(CranberryOS_WARNINGS_AS_ERRORS "Warnings as errors" ON)
    option(CranberryOS_ENABLE_LTO "Link-time Optimization" ON)
endmacro()

macro(CranberryOS_Options)
    if(CranberryOS_ENABLE_LTO)
        include(cmake/LinkTimeOptimization.cmake)
        CranberryOS_EnableLTO()
    endif()

    include(cmake/StandardProjectSettings.cmake)

    # Setting up dummy "libraries" to hang properties off of
    add_library(CranberryOS_Warnings INTERFACE)
    add_library(CranberryOS_Options INTERFACE)

    include(cmake/CompilerWarnings.cmake)
    CranberryOS_SetProjectWarnings(CranberryOS_Warnings ${CranberryOS_WARNINGS_AS_ERRORS})

    if(CranberryOS_WARNINGS_AS_ERRORS)
        check_cxx_compiler_flag("-Wl,--fatal-warnings" linkerFatalWarnings)
        if(linkerFatalWarnings)
            target_link_options(CranberryOS_Options INTERFACE -Wl,--fatal-warnings)
        endif()
    endif()

    # #TODO: We should look into clang-tidy
endmacro()