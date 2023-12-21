set(LINKER_SCRIPT "link.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT}")

add_executable(kernel8.elf
    Exceptions.cpp
    ExceptionVectorHandlers.h ExceptionVectorHandlers.cpp
    ExceptionVectors.S
    IRQ.h IRQ.S
    Main.h Main.cpp
    MemoryManager.h MemoryManager.cpp MemoryManager.S
    MiniUart.h MiniUart.cpp
    Print.h Print.cpp
    Scheduler.h Scheduler.cpp Scheduler.S
    Start.S
    SystemCall.cpp
    SystemCallDefines.h
    TaskStructs.h
    Timer.h Timer.cpp
    TypeInfo.cpp
    UnitTests.h UnitTests.cpp
    user_Program.h user_Program.cpp
    user_SystemCall.h user_SystemCall.cpp user_SystemCall.S
    Utils.h Utils.cpp
)
set_target_properties(kernel8.elf PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT}")
target_compile_features(kernel8.elf PUBLIC cxx_std_17)

# Bring in our standard library
target_link_libraries(kernel8.elf PRIVATE kernel_stdlib CranberryOS_Options CranberryOS_Warnings)

# Bring in each sub folder that we need (they have their own CMakeLists.txt files that will add appropriate
# information to the targets)
add_subdirectory(AArch64)
add_subdirectory(Peripherals)

# GPU can't read elf files, so we have to extract the contents out to an img file so it can blindly load the file and
# start execution at the expected offset
add_custom_command(
    TARGET kernel8.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
    ARGS -O binary kernel8.elf kernel8.img
    BYPRODUCTS kernel8.img
    COMMENT "Generating img file"
)
