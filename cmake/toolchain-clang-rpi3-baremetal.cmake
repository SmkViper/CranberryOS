set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  arm)

# Without that flag CMake is not able to pass test compilation check as it can't seem to correctly do linking for cross compilation
set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)

# Not a fan of having these be two things, but lld uses a different triple...
set(triple aarch64-elf)
set(ldtriple aarch64elf)

# Putting the compiler into the cache so VSCode CMake Tools can read them (see CMake issue 20225)
set(CMAKE_ASM_COMPILER  clang CACHE INTERNAL "")
set(CMAKE_C_COMPILER    clang CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER  clang++ CACHE INTERNAL "")
set(CMAKE_OBJCOPY       llvm-objcopy)

set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(CMAKE_C_FLAGS_INIT           "-ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53+nosimd")
set(CMAKE_CXX_FLAGS_INIT         "${CMAKE_C_FLAGS} -nostdinc++ -fno-rtti")

set(CMAKE_EXE_LINKER_FLAGS  "-fuse-ld=lld -nostdlib" CACHE INTERNAL "")

# Make sure we don't try to get anything outside of this folder for libraries, includes, etc because they likely won't be built
# for arm, but for the host build system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
