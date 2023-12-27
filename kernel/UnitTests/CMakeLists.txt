target_sources(kernel8.elf
    PRIVATE
        Framework.h Framework.cpp
        MemoryManagerTests.h MemoryManagerTests.cpp
        PrintTests.h PrintTests.cpp
)

add_subdirectory(KernelStdlib)
