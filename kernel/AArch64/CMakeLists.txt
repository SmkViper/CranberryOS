target_sources(kernel8.elf
    PRIVATE
        CPU.h CPU.cpp
        ExceptionVectorDefines.h
        MemoryDescriptor.h MemoryDescriptor.cpp
        MemoryPageTables.h
        RegisterDefines.h
        SchedulerDefines.h
        SystemRegisters.h SystemRegisters.cpp
)

add_subdirectory(Boot)
