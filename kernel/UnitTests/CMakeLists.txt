target_sources(kernel8.elf
    PRIVATE
        Framework.h Framework.cpp
        MemoryManagerTests.h MemoryManagerTests.cpp
)

add_subdirectory(KernelStdlib)
