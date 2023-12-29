target_sources(kernel8.elf
    PRIVATE
        CPUTests.h CPUTests.cpp
        MemoryDescriptorTests.h MemoryDescriptorTests.cpp
        MemoryPageTablesTests.h MemoryPageTablesTests.cpp
        SystemRegistersTests.h SystemRegistersTests.cpp
)
