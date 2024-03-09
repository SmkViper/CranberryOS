target_sources(kernel8.elf
    PRIVATE
        BitsetTests.h BitsetTests.cpp
        BitTests.cpp
        CLimitsTests.cpp
        CStdDefTests.cpp
        CStdIntTests.cpp
        CStringTests.h CStringTests.cpp
        ExceptionTests.h ExceptionTests.cpp
        NewTests.h NewTests.cpp
        TypeInfoTests.h TypeInfoTests.cpp
        TypeTraitsTests.cpp
        UtilityTests.h UtilityTests.cpp
)