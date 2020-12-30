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
# For whatever reason I can't figure out how to get Clang to correctly pass the target triple to the linker, so we're just going to
# do the stupid thing for now and force CMake to use ld.lld directly with it's own flags and target triple
set(CMAKE_C_LINK_EXECUTABLE "ld.lld <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "ld.lld <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(CMAKE_C_FLAGS           "-ffreestanding -nostdinc -nostdlib -mtune=cortex-a53+nosimd" CACHE INTERNAL "")
set(CMAKE_CXX__FLAGS        "<CMAKE_C_FLAGS>" -nostdinc++ CACHE INTERNAL "")

# Aborted attempt to get clang to run lld as the linker, but it seemed to refuce to pass the target triple through, resulting in
# errors related to trying to link as a x86-64 elf, not the arm one.
#set(CMAKE_EXE_LINKER_FLAGS  "-fuse-ld=lld" CACHE INTERNAL "")

set(CMAKE_EXE_LINKER_FLAGS "-m ${ldtriple} -nostdlib" CACHE INTERNAL "")

# Make sure we don't try to get anything outside of this folder for libraries, includes, etc because they likely won't be built
# for arm, but for the host build system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
