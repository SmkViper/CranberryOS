target_sources(kernel8.elf
    PRIVATE
        Boot.cpp
        ExceptionLevel.h ExceptionLevel.cpp
        MMU.h MMU.cpp
        Output.h Output.cpp
)
