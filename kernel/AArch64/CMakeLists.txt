target_sources(kernel8.elf
    PRIVATE
        ExceptionVectorDefines.h
        MMUDefines.h
        RegisterDefines.h
        SchedulerDefines.h
        SystemRegisters.h
)

add_subdirectory(Boot)
