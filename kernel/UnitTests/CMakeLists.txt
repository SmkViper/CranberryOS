target_sources(kernel8.elf
    PRIVATE
        Framework.h Framework.cpp
        MemoryManagerTests.h MemoryManagerTests.cpp
        PointerTypesTests.h PointerTypesTests.cpp
        PrintTests.h PrintTests.cpp
        UtilsTests.h UtilsTests.cpp
)

add_subdirectory(AArch64)
add_subdirectory(KernelStdlib)
