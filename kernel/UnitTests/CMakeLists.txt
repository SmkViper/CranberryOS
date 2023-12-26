target_sources(kernel8.elf
    PRIVATE
        Framework.h Framework.cpp
)

add_subdirectory(KernelStdlib)
