if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified")
    set(CMAKE_BUILD_TYPE
        RelWithDebInfo
        CACHE STRING "Choose the type of build" FORCE
    )
    set_property(
        CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS
            "Debug"
            "Release"
            "MinSizeRel"
            "RelWithDebInfo"
    )
endif()

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# VSCode mangles the colored output because it doesn't support it:
# https://github.com/microsoft/vscode-cmake-tools/issues/478

#if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
#    add_compile_options(-fcolor-diagnostics)
#else()
#    message(STATUS "No colored compiler diagnostic set for '${CMAKE_CXX_COMPILER_ID}' compiler")
#endif()