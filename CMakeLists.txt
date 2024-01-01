cmake_minimum_required(VERSION 3.18)

# set the C++ standard version if not already set
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

project(CranberryOS VERSION 1.0
                    DESCRIPTION "Silly little toy OS for a Raspberry Pi"
                    LANGUAGES C CXX ASM)

include(ProjectOptions.cmake)

CranberryOS_SetupOptions()
CranberryOS_Options()

target_compile_features(CranberryOS_Options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_subdirectory(kernel_stdlib)
add_subdirectory(kernel)
